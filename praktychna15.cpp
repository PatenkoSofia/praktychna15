#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Node {
    int value;
    Node* left;
    Node* right;
};

std::size_t allocatedNodes = 0;
std::size_t freedNodes = 0;

std::size_t activeNodes() {
    return allocatedNodes - freedNodes;
}

Node* createNode(int value) {
    Node* node = new Node{value, nullptr, nullptr};
    ++allocatedNodes;
    return node;
}

void deleteNode(Node* node) {
    delete node;
    ++freedNodes;
}

void destroyTree(Node* node) {
    if (node == nullptr) {
        return;
    }
    destroyTree(node->left);
    destroyTree(node->right);
    deleteNode(node);
}

void clearTree(Node*& root) {
    destroyTree(root);
    root = nullptr;
}

bool isEmpty(const Node* root) {
    return root == nullptr;
}

Node* readPrefix(std::istream& in) {
    std::string token;
    if (!(in >> token)) {
        throw std::runtime_error("неочікуваний кінець опису дерева");
    }
    if (token == "#") {
        return nullptr;
    }
    std::size_t used = 0;
    const int value = std::stoi(token, &used);
    if (used != token.size()) {
        throw std::runtime_error("некоректне значення: " + token);
    }
    Node* node = createNode(value);
    try {
        node->left = readPrefix(in);
        node->right = readPrefix(in);
    } catch (...) {
        clearTree(node);
        throw;
    }
    return node;
}

void writePrefix(const Node* node, std::vector<std::string>& tokens) {
    if (node == nullptr) {
        tokens.push_back("#");
        return;
    }
    tokens.push_back(std::to_string(node->value));
    writePrefix(node->left, tokens);
    writePrefix(node->right, tokens);
}

std::size_t countNodes(const Node* node) {
    if (node == nullptr) {
        return 0;
    }
    return 1 + countNodes(node->left) + countNodes(node->right);
}

std::size_t countLeaves(const Node* node) {
    if (node == nullptr) {
        return 0;
    }
    if (node->left == nullptr && node->right == nullptr) {
        return 1;
    }
    return countLeaves(node->left) + countLeaves(node->right);
}

std::size_t height(const Node* node) {
    if (node == nullptr) {
        return 0;
    }
    return 1 + std::max(height(node->left), height(node->right));
}

void inorderRec(const Node* node, std::vector<int>& out) {
    if (node == nullptr) {
        return;
    }
    inorderRec(node->left, out);
    out.push_back(node->value);
    inorderRec(node->right, out);
}

std::vector<int> inorder(const Node* root) {
    std::vector<int> out;
    inorderRec(root, out);
    return out;
}

class ArrayStack {
public:
    explicit ArrayStack(std::size_t capacity)
        : data_(new const Node*[capacity]), capacity_(capacity), size_(0) {}
    ~ArrayStack() {
        delete[] data_;
    }
    ArrayStack(const ArrayStack&) = delete;
    ArrayStack& operator=(const ArrayStack&) = delete;

    void push(const Node* node) {
        if (size_ == capacity_) {
            throw std::runtime_error("переповнення допоміжного стека");
        }
        data_[size_++] = node;
    }
    const Node* pop() {
        return data_[--size_];
    }
    bool empty() const {
        return size_ == 0;
    }

private:
    const Node** data_;
    std::size_t capacity_;
    std::size_t size_;
};

std::vector<int> inorderIterative(const Node* root) {
    std::vector<int> out;
    ArrayStack stack(countNodes(root) + 1);
    const Node* current = root;
    while (current != nullptr || !stack.empty()) {
        while (current != nullptr) {
            stack.push(current);
            current = current->left;
        }
        current = stack.pop();
        out.push_back(current->value);
        current = current->right;
    }
    return out;
}

struct SearchResult {
    const Node* found;
    std::vector<int> path;
    std::size_t comparisons;
};

SearchResult searchIterative(const Node* root, int key) {
    SearchResult result{nullptr, {}, 0};
    const Node* current = root;
    while (current != nullptr) {
        result.path.push_back(current->value);
        ++result.comparisons;
        if (key == current->value) {
            result.found = current;
            return result;
        }
        current = key < current->value ? current->left : current->right;
    }
    return result;
}

const Node* searchRec(const Node* node, int key, std::vector<int>& path, std::size_t& comparisons) {
    if (node == nullptr) {
        return nullptr;
    }
    path.push_back(node->value);
    ++comparisons;
    if (key == node->value) {
        return node;
    }
    return key < node->value ? searchRec(node->left, key, path, comparisons)
                             : searchRec(node->right, key, path, comparisons);
}

bool insertKey(Node*& root, int key) {
    Node** link = &root;
    while (*link != nullptr) {
        if (key == (*link)->value) {
            return false;
        }
        link = key < (*link)->value ? &(*link)->left : &(*link)->right;
    }
    *link = createNode(key);
    return true;
}

enum class RemoveCase { NotFound, Leaf, OneChild, TwoChildren };

RemoveCase removeKey(Node*& root, int key) {
    Node** link = &root;
    while (*link != nullptr && (*link)->value != key) {
        link = key < (*link)->value ? &(*link)->left : &(*link)->right;
    }
    Node* target = *link;
    if (target == nullptr) {
        return RemoveCase::NotFound;
    }
    if (target->left != nullptr && target->right != nullptr) {
        Node** predLink = &target->left;
        while ((*predLink)->right != nullptr) {
            predLink = &(*predLink)->right;
        }
        Node* predecessor = *predLink;
        target->value = predecessor->value;
        *predLink = predecessor->left;
        deleteNode(predecessor);
        return RemoveCase::TwoChildren;
    }
    const bool leaf = target->left == nullptr && target->right == nullptr;
    *link = target->left != nullptr ? target->left : target->right;
    deleteNode(target);
    return leaf ? RemoveCase::Leaf : RemoveCase::OneChild;
}

std::string removeCaseName(RemoveCase kind) {
    switch (kind) {
    case RemoveCase::NotFound:
        return "ключ не знайдено";
    case RemoveCase::Leaf:
        return "випадок листка";
    case RemoveCase::OneChild:
        return "випадок одного нащадка";
    case RemoveCase::TwoChildren:
        return "випадок двох нащадків";
    }
    return "";
}

bool validateRange(const Node* node, bool hasLow, int low, bool hasHigh, int high,
                   std::size_t depth, std::string& error) {
    if (node == nullptr) {
        return true;
    }
    if (hasLow && node->value <= low) {
        error = "ключ " + std::to_string(node->value) + " на глибині " + std::to_string(depth)
            + " порушує межу: має бути більше " + std::to_string(low);
        return false;
    }
    if (hasHigh && node->value >= high) {
        error = "ключ " + std::to_string(node->value) + " на глибині " + std::to_string(depth)
            + " порушує межу: має бути менше " + std::to_string(high);
        return false;
    }
    return validateRange(node->left, hasLow, low, true, node->value, depth + 1, error)
        && validateRange(node->right, true, node->value, hasHigh, high, depth + 1, error);
}

bool validate(const Node* root, std::string& error) {
    error.clear();
    return validateRange(root, false, 0, false, 0, 0, error);
}

std::string format(const std::vector<int>& values) {
    if (values.empty()) {
        return "порожньо";
    }
    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << values[i];
    }
    return out.str();
}

std::string join(const std::vector<std::string>& tokens) {
    std::string result;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i != 0) {
            result += ' ';
        }
        result += tokens[i];
    }
    return result;
}

void printTree(const Node* node, std::size_t depth) {
    if (node == nullptr) {
        return;
    }
    printTree(node->right, depth + 1);
    std::cout << std::string(depth * 4, ' ') << node->value << '\n';
    printTree(node->left, depth + 1);
}

struct Op {
    char kind;
    int key;
    bool expectOk;
    std::size_t expectPath;
};

struct Scenario {
    int number;
    const char* description;
    std::vector<int> initial;
    std::vector<Op> ops;
    std::vector<int> finalInorder;
};

std::string opName(const Op& op) {
    if (op.kind == 'S') {
        return "пошук " + std::to_string(op.key);
    }
    if (op.kind == 'I') {
        return "вставлення " + std::to_string(op.key);
    }
    if (op.kind == 'D') {
        return "видалення " + std::to_string(op.key);
    }
    return "очищення";
}

const Scenario kScenarios[] = {
    {1, "пошук у порожньому дереві", {}, {{'S', 5, false, 0}}, {}},
    {2, "вставлення в порожнє дерево", {}, {{'I', 5, true, 0}}, {5}},
    {3, "видалення єдиного кореня", {5}, {{'D', 5, true, 0}}, {}},
    {4, "вставлення листка", {5, 3, 7}, {{'I', 4, true, 0}}, {3, 4, 5, 7}},
    {5, "вставлення повторюваного ключа", {5, 3, 7}, {{'I', 5, false, 0}}, {3, 5, 7}},
    {6, "видалення листка", {5, 3, 7}, {{'D', 3, true, 0}}, {5, 7}},
    {7, "видалення вузла з одним нащадком", {5, 3, 7, 6}, {{'D', 7, true, 0}}, {3, 5, 6}},
    {8, "видалення вузла з двома нащадками", {5, 3, 8, 7, 9}, {{'D', 8, true, 0}}, {3, 5, 7, 9}},
    {9, "видалення кореня з двома нащадками", {5, 3, 8, 7, 9}, {{'D', 5, true, 0}}, {3, 7, 8, 9}},
    {10, "видалення відсутнього ключа", {5, 3, 7}, {{'D', 10, false, 0}}, {3, 5, 7}},
    {11, "пошук у вироджено-правому дереві", {1, 2, 3, 4}, {{'S', 4, true, 4}}, {1, 2, 3, 4}},
    {12, "пошук у збалансованому дереві", {4, 2, 6, 1, 3, 5, 7}, {{'S', 7, true, 3}}, {1, 2, 3, 4, 5, 6, 7}},
    {13, "видалення кореня з двома нащадками", {5, 3, 7, 6}, {{'D', 5, true, 0}}, {3, 6, 7}},
    {14, "видалення кореня з одним нащадком", {2, 1}, {{'D', 2, true, 0}}, {1}},
    {15, "видалення кореня з одним нащадком праворуч", {2, 3}, {{'D', 2, true, 0}}, {3}},
    {16, "повторне видалення того самого ключа", {5, 3, 7}, {{'D', 3, true, 0}, {'D', 3, false, 0}}, {5, 7}},
    {17, "замінник з одним нащадком", {10, 6, 4, 5, 15}, {{'D', 10, true, 0}}, {4, 5, 6, 15}},
    {18, "три послідовні видалення кореня", {4, 2, 6, 1, 3, 5, 7},
     {{'D', 4, true, 0}, {'D', 3, true, 0}, {'D', 2, true, 0}}, {1, 5, 6, 7}},
    {19, "очищення та повторне створення", {5, 3, 7},
     {{'C', 0, true, 0}, {'I', 5, true, 0}, {'I', 3, true, 0}, {'I', 7, true, 0}}, {3, 5, 7}},
};

bool checkAfterOperation(const Node* root, std::vector<std::string>& errors) {
    std::string error;
    if (!validate(root, error)) {
        errors.push_back("порушення властивості дерева пошуку: " + error);
        return false;
    }
    return true;
}

bool runScenario(const Scenario& sc) {
    std::vector<std::string> errors;
    Node* root = nullptr;
    for (int key : sc.initial) {
        if (!insertKey(root, key)) {
            errors.push_back("початкове вставлення " + std::to_string(key));
        }
    }

    std::ostringstream log;
    log << "№" << sc.number << " " << sc.description << ": ";
    for (const Op& op : sc.ops) {
        const std::vector<int> beforeInorder = inorder(root);
        const std::size_t beforeNodes = countNodes(root);
        bool ok = false;
        std::string text;

        if (op.kind == 'S') {
            const SearchResult fast = searchIterative(root, op.key);
            std::vector<int> recPath;
            std::size_t recComparisons = 0;
            const Node* recFound = searchRec(root, op.key, recPath, recComparisons);
            ok = fast.found != nullptr;
            if ((recFound != nullptr) != ok || recPath != fast.path || recComparisons != fast.comparisons) {
                errors.push_back("рекурсивний і ітеративний пошук різняться");
            }
            if (inorder(root) != beforeInorder || countNodes(root) != beforeNodes) {
                errors.push_back("пошук змінив дерево");
            }
            if (op.expectPath != 0 && fast.path.size() != op.expectPath) {
                errors.push_back("довжина шляху пошуку");
            }
            text = std::string(ok ? "знайдено" : "не знайдено") + ", шлях " + format(fast.path)
                + ", порівнянь " + std::to_string(fast.comparisons);
        } else if (op.kind == 'I') {
            ok = insertKey(root, op.key);
            if (ok) {
                if (countNodes(root) != beforeNodes + 1) {
                    errors.push_back("лічильник вузлів після вставлення");
                }
                text = "вставлено";
            } else {
                if (inorder(root) != beforeInorder) {
                    errors.push_back("невдала вставка змінила дерево");
                }
                text = "повторюваний ключ, дерево без змін";
            }
        } else if (op.kind == 'D') {
            const RemoveCase kind = removeKey(root, op.key);
            ok = kind != RemoveCase::NotFound;
            if (ok) {
                if (countNodes(root) + 1 != beforeNodes) {
                    errors.push_back("лічильник вузлів після видалення");
                }
                text = "видалено, " + removeCaseName(kind);
            } else {
                if (inorder(root) != beforeInorder) {
                    errors.push_back("невдале видалення змінило дерево");
                }
                text = removeCaseName(kind) + ", дерево без змін";
            }
        } else {
            clearTree(root);
            ok = true;
            text = "дерево очищено";
        }

        if (ok != op.expectOk) {
            errors.push_back("статус операції " + opName(op));
        }
        checkAfterOperation(root, errors);
        log << "[" << opName(op) << ": " << text << "; обхід " << format(inorder(root)) << "] ";
    }

    if (inorder(root) != sc.finalInorder) {
        errors.push_back("фінальний симетричний обхід");
    }
    if (inorderIterative(root) != inorder(root)) {
        errors.push_back("ітеративний і рекурсивний обходи різняться");
    }

    clearTree(root);
    if (!isEmpty(root)) {
        errors.push_back("корінь не порожній після очищення");
    }

    std::cout << log.str() << (errors.empty() ? "OK" : "ПОМИЛКА") << '\n';
    for (const std::string& error : errors) {
        std::cout << "    помилка: " << error << '\n';
    }
    return errors.empty();
}

bool runValidationScenario() {
    Node* good = nullptr;
    const int keys[] = {5, 3, 7};
    for (int key : keys) {
        insertKey(good, key);
    }
    std::string goodError;
    const bool goodValid = validate(good, goodError);
    clearTree(good);

    std::istringstream in("5 3 6 # # # #");
    Node* bad = readPrefix(in);
    std::string badError;
    const bool badValid = validate(bad, badError);
    clearTree(bad);

    const bool ok = goodValid && !badValid
        && badError.find("на глибині 2") != std::string::npos && activeNodes() == 0;
    std::cout << "№20 перевірка інваріантів: правильне дерево " << (goodValid ? "OK" : "ПОМИЛКА")
              << "; дерево з помилкою на глибині 2: " << badError << "; "
              << (ok ? "OK" : "ПОМИЛКА") << '\n';
    return ok;
}

bool runVariant() {
    const int sequence[] = {49, 24, 67, 11, 35, 57, 80};
    Node* root = nullptr;
    bool ok = true;
    for (int key : sequence) {
        ok = insertKey(root, key) && ok;
    }

    std::string error;
    std::vector<std::string> tokens;
    writePrefix(root, tokens);
    std::cout << "Варіант 22. Порядок створення: 49, 24, 67, 11, 35, 57, 80\n";
    std::cout << "Префіксний опис: " << join(tokens) << '\n';
    std::cout << "Після створення: симетричний обхід " << format(inorder(root))
              << ", інваріанти " << (validate(root, error) ? "OK" : "ПОМИЛКА") << '\n';
    ok = ok && validate(root, error) && inorder(root) == std::vector<int>({11, 24, 35, 49, 57, 67, 80});

    const SearchResult found = searchIterative(root, 35);
    std::cout << "Команда 1, пошук 35: " << (found.found != nullptr ? "знайдено" : "не знайдено")
              << ", шлях " << format(found.path) << ", порівнянь " << found.comparisons << '\n';
    std::cout << "    після пошуку: симетричний обхід " << format(inorder(root))
              << ", інваріанти " << (validate(root, error) ? "OK" : "ПОМИЛКА") << '\n';
    ok = ok && found.found != nullptr && found.comparisons == 3;

    const bool inserted = insertKey(root, 60);
    std::cout << "Команда 2, вставлення 60: " << (inserted ? "вставлено" : "повторюваний ключ")
              << "; симетричний обхід " << format(inorder(root))
              << ", інваріанти " << (validate(root, error) ? "OK" : "ПОМИЛКА") << '\n';
    ok = ok && inserted && validate(root, error);

    const RemoveCase removed = removeKey(root, 24);
    std::cout << "Команда 3, видалення 24: " << removeCaseName(removed)
              << "; симетричний обхід " << format(inorder(root))
              << ", інваріанти " << (validate(root, error) ? "OK" : "ПОМИЛКА") << '\n';
    ok = ok && removed == RemoveCase::TwoChildren && validate(root, error)
        && inorder(root) == std::vector<int>({11, 35, 49, 57, 60, 67, 80});

    std::cout << "Дерево після трьох команд:\n";
    printTree(root, 0);
    std::cout << "Вузлів: " << countNodes(root) << ", листків: " << countLeaves(root)
              << ", висота: " << height(root) << '\n';

    clearTree(root);
    std::cout << "Після очищення активних вузлів: " << activeNodes() << '\n';
    return ok && activeNodes() == 0;
}

bool runExperiment() {
    struct Shape {
        const char* name;
        std::vector<int> order;
        std::size_t expectHeight;
        std::size_t expectFound;
        std::size_t expectMissing;
    };
    const Shape shapes[] = {
        {"майже збалансована", {4, 2, 6, 1, 3, 5, 7}, 3, 3, 3},
        {"вироджена зростаюча", {1, 2, 3, 4, 5, 6, 7}, 7, 7, 7},
        {"змішана", {4, 7, 2, 6, 1, 5, 3}, 4, 2, 2},
    };
    bool ok = true;
    std::cout << "Експеримент: 7 однакових ключів, різний порядок вставлення\n";
    for (const Shape& shape : shapes) {
        Node* root = nullptr;
        for (int key : shape.order) {
            insertKey(root, key);
        }
        const SearchResult found = searchIterative(root, 7);
        const SearchResult missing = searchIterative(root, 8);
        const std::size_t h = height(root);
        const bool same = h == shape.expectHeight && found.comparisons == shape.expectFound
            && missing.comparisons == shape.expectMissing && found.found != nullptr && missing.found == nullptr;
        ok = ok && same;
        std::cout << "Форма: " << shape.name << "; висота " << h << "; пошук наявного 7: "
                  << found.comparisons << " порівнянь; пошук відсутнього 8: "
                  << missing.comparisons << " порівнянь; " << (same ? "OK" : "ПОМИЛКА") << '\n';
        clearTree(root);
    }
    return ok && activeNodes() == 0;
}

}

int main() {
    try {
        std::cout << "Практична робота 15. Пошук, вставлення та видалення в бінарному дереві\n\n";
        const bool variantOk = runVariant();

        std::cout << "\nАвтоматичні сценарії: 15 табличних і 5 власних\n";
        std::size_t passed = 0;
        const std::size_t total = std::size(kScenarios) + 1;
        for (const Scenario& sc : kScenarios) {
            if (runScenario(sc)) {
                ++passed;
            }
        }
        if (runValidationScenario()) {
            ++passed;
        }

        std::cout << "\nЕксперимент з формою дерева\n";
        const bool experimentOk = runExperiment();

        std::cout << "\nПройдено сценаріїв: " << passed << " з " << total << '\n';
        std::cout << "Створено вузлів: " << allocatedNodes << ", звільнено: " << freedNodes
                  << ", активних: " << activeNodes() << '\n';

        const bool allOk = variantOk && experimentOk && passed == total && activeNodes() == 0;
        std::cout << (allOk ? "Підсумок: успішно" : "Підсумок: є помилки") << '\n';
        return allOk ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Помилка: " << e.what() << '\n';
        return 1;
    }
}
