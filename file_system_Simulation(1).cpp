#include <iostream>        
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <unordered_map>
#include <sstream>
using namespace std;

struct Node {                                               // a file or folder
    string name;
    bool isFolder;
    int size;                                               // size in KB (files only)
    Node* parent;                      
    Node* head;                                             // first child(linked list) of this folder
    Node* tail;                                             // last child(in the linked list) of this folder
    Node* next;                                             // next sibling, but part of same folder
    Node* prev;                                             // previous sibling but part of same folder; same leve;l, but different folders not counted
    unordered_map<string, Node*> lookup;                    // hash table: name -> child UNDERSTAND

    Node(const string& n, bool folder, int s = 0)           //initialiser (s=0: as folders have no size; this is applicable only for files)
        : name(n), isFolder(folder), size(s),               
          parent(nullptr), head(nullptr), tail(nullptr),
          next(nullptr), prev(nullptr) {}
};

Node* root = nullptr;                                      // root is the absolute top of the tree
Node* cwd  = nullptr;                                      // the folder we are standing in (current working directory)

                                                         
void adopt(Node* parent, Node* child) {                   // Add child at the tail of parent's linked list. O(1)
    child->parent = parent;                               // setting the child'd parent to the folder we want to add a child in
    child->next = nullptr;                                // since it joins at the end, there is no next sibling
    child->prev = parent->tail;                           // as there is a new child, the child's previous is set to the previous end of the folder
    if (parent->tail) {
        parent->tail->next = child;                       // if the folder already has children, the previous end's next is set to the new child
            }        
    else{
        parent->head = child;                             // if the folder has no children, the head is set to the new child
    }                           
    parent->tail = child;                                 // the tail of the folder is set to the new child
    parent->lookup[child->name] = child;                  // register in hash table UNDERSTAND
}

                                                         
void disown(Node* child) {                              // disown a child (undo can restore it)
    Node* p = child->parent;                            // parent is stored, so that undo can fetch this
    if (child->prev){                                   // if the child has a previous sibling, the previous sibling's next is set to the child's next
         child->prev->next = child->next;
    }    
    else{                                               // if the child has no previous sibling, the head of the folder is set to the child's next
         p->head = child->next;
    }
    if (child->next){                                   // if the child has a next sibling, the next sibling's previous is set to the child's previous
         child->next->prev = child->prev;
    }
    else{                                               // if the child has no next sibling, the tail of the folder is set to the child's previous
         p->tail = child->prev;
    }
    p->lookup.erase(child->name);
    child->prev = child->next = nullptr;                // Child next and previous set to nullptr, so now it is disowned
}


void destroy(Node* n) {                                 // recursively abort all children and then delete the parent
    Node* c = n->head;                                  // start with the first child of the folder
    while (c) {
        Node* nx = c->next;                             // store the next sibling, so that we can move to it after aborting the current child
        destroy(c);                                     // recursively abort the child
        c = nx;                                         // move to the next sibling
    }
    delete n;                                          // delete the parent after all children have been aborted
}

// Is 'ancestor' equal to n, or somewhere above n? Walk up via parent.
bool isInside(Node* ancestor, Node* n) {              // check if ancestor is equal to n or somewhere above n
    while (n) {                                       // while n is not nullptr, keep checking
        if (n == ancestor){                           // if n is equal to ancestor, return true
             return true;
        }
        n = n->parent;                                // move up to the parent of n
    }
    return false;                                     // if we reach the root and haven't found ancestor, return false
}


string getPath(Node* n) {                            // get the full path of a node by walking up to the root and pushing names onto a stack
    if (n == root){                                  // if n is the root, return "/"
         return "/";
    }
    stack<string> st;                               // create a stack to hold the names of the nodes as we walk up to the root
    while (n->parent) {                             // while n has a parent, keep pushing names onto the stack
        st.push(n->name);                           // push the name of the current node onto the stack
        n = n->parent;                              // move up to the parent of n
    }
    string path;                                    // create a string to hold the full path
    while (!st.empty()) {                           // while the stack is not empty, pop names off the stack and add them to the path
        path += "/" + st.top();                     // add a "/" before the name to create the full path
        st.pop();                                   // pop the name off the stack
    }
    return path;                                   // return the full path as a string
}


vector<string> split(const string& s, char delim) { // split a string into parts(different folders/files) based on a delimiter character
    vector<string> parts;                           // create a vector to hold the parts of the string
    string cur;                                     // create a string to hold the current part of the string
    for (char ch : s) {                             // for each character in the string, check if it is the delimiter
        if (ch == delim) {                          // if it is the delimiter, push the current part onto the vector and clear the current part
            if (!cur.empty()) parts.push_back(cur); // if the current part is not empty, push it onto the vector
            cur.clear();                            // clear the current part to start a new part
        } 
        else{                                       // if it is not the delimiter, add the character to the current part
             cur += ch;                             
        }
    }
    if (!cur.empty()) parts.push_back(cur);         // if the current part is not empty, push it onto the vector
    return parts;                                   // return the vector of parts as a vector of strings
}


Node* resolve(const string& path) {                               // it makes use of the split function to break the path into parts and 
                                                                  // then traverses the tree to find the node corresponding to the path
    Node* cur = (!path.empty() && path[0] == '/') ? root : cwd;   // if the path starts with a '/', start from the root, 
                                                                  // otherwise start from the current working directory (cwd)
    vector<string> parts = split(path, '/');                      // split the path into parts using '/' as the delimiter
    for (const string& p : parts) {                               // for each part of the path, check if it is "." or ".." or a valid name
        if (p == ".") continue;                                   // if the part is ".", continue to the next part
        if (p == "..") {                                          // if the part is "..", move up to the parent of the current node, if it exists
            if (cur->parent) cur = cur->parent;                   // if the current node has a parent, move up to the parent
            else return nullptr;                                  // if the current node does not have a parent, return nullptr (invalid path)
            continue;                                             // continue to the next part
        }
        if (!cur->isFolder) return nullptr;                       // if the current node is not a folder, return nullptr (invalid path)
        auto it = cur->lookup.find(p);           // O(1) average  // look up the part in the current node's lookup table (hash table) to find the corresponding child node
        if (it == cur->lookup.end()) return nullptr;              // if the part is not found in the lookup table, return nullptr (invalid path)
        cur = it->second;                                         // if the part is found in the lookup table, move to the corresponding child node
    }
    return cur;                                                   // return the node corresponding to the path, or nullptr if the path is invalid
}


enum ActionType { CREATE, REMOVE };                              // this function defines an enum called ActionType with two possible values: CREATE and REMOVE.
struct Action {                                                  // this function defines a struct called Action that contains three members: 
                                                                 // an ActionType called type, a pointer to a Node called node, and a pointer to a Node called parent.
    ActionType type;                                             
    Node* node;                                                  
    Node* parent;
};
stack<Action> undoStack;                                        // this function defines a stack of Action objects called undoStack, 
                                                                // which will be used to keep track of the actions performed on the file system, 
                                                                // so that they can be undone if necessary.   


struct BSTNode {                                                // this function defines a struct called BSTNode that contains four members:
    string key;                                                 // a string called key, a pointer to a Node called ref, and two pointers to BSTNode called left and right
    Node* ref;
    BSTNode *left, *right;
    BSTNode(const string& k, Node* r) : key(k), ref(r), left(nullptr), right(nullptr) {} 
                                                                // constructor that initializes the key and ref members, 
                                                                // and sets the left and right pointers to nullptr
};

BSTNode* bstInsert(BSTNode* r, Node* n) {                       // this function inserts a new node into a binary search tree (BST) based on the name of the node
    if (!r) return new BSTNode(n->name, n);                     // if the root of the BST is nullptr, create a new BSTNode with the name of the node and return it as the new root
    if (n->name < r->key) r->left  = bstInsert(r->left, n);     // if the name of the node is alphabetically less than the key of the current BSTNode, recursively call bstInsert on the left subtree
    else{
        r->right = bstInsert(r->right, n);                      // if the name of the node is alphabetically greater than or equal to the key of the current BSTNode, recursively call bstInsert on the right subtree
    }
    return r;                                                   // return the root of the BST after the insertion is complete
}

void printNodeLine(Node* n) {                                  // this function prints the name and size of a node in a formatted way, 
                                                               // depending on whether it is a folder or a file
    if (n->isFolder){                                          // if the node is a folder, print "[D]" followed by the name of the folder
         cout << "  [D] " << n->name << "\n"; 
    }
    else{                                                     // if the node is a file, print "[F]" followed by the name of the file and its size in KB
        cout << "  [F] " << n->name << "  (" << n->size << " KB)\n";
    }
}

// Inorder traversal of a BST gives keys in sorted order.
void bstInorder(BSTNode* r) {                   // this function performs an inorder traversal of a binary search tree (BST) 
                                                // and prints the names and sizes of the nodes in sorted order
    if (!r) return;                             // if the root of the BST is nullptr, return (base case)
    bstInorder(r->left);                        // recursively call bstInorder on the left subtree
    printNodeLine(r->ref);                      // print the name and size of the current node using the printNodeLine function
    bstInorder(r->right);                       // recursively call bstInorder on the right subtree
}

void bstFree(BSTNode* r) {                      // this function frees the memory used by a binary search tree (BST) 
                                                // by recursively deleting all the nodes in the tree
    if (!r) return;                             // if the root of the BST is nullptr, return (base case)    
    bstFree(r->left);                           // recursively call bstFree on the left subtree
    bstFree(r->right);                          // recursively call bstFree on the right subtree
    delete r;                                   // delete the current node to free its memory
}


bool validName(const string& name) {                        // this function checks if a given name is valid for a file or folder in the file system
    return !name.empty() && name != "." && name != ".." &&  // the name is not empty, and it is not "." or "..", and it does not contain any '/' characters
           name.find('/') == string::npos;                  // the name does not contain any '/' characters, which are reserved for separating folder names in paths
}

void cmdCreate(const string& name, bool folder, int size) { // this function creates a new file or folder in the current working directory (cwd) 
                                                            // with the given name, type (folder or file), and size (for files)
    if (!validName(name)) {                                // check if the name is valid using the validName function, and print an error message if it is not
        cout << "Invalid name.\n"; return; 
    } 
    if (cwd->lookup.count(name)) {                        // check if a file or folder with the same name already exists in the cwd using the lookup hash table, and print an error message if it does
        cout << "'" << name << "' already exists here.\n"; 
        return; 
    }
    Node* n = new Node(name, folder, size);               // create a new Node with the given name, type, and size
    adopt(cwd, n);                                        // add the new Node as a child of the cwd using the adopt function
    undoStack.push({CREATE, n, cwd});                     // push an Action onto the undoStack to allow undoing the creation of the new Node
}

void cmdLs(bool sorted) {                             // this function lists the contents of the current working directory (cwd)
                                                      // in either sorted or unsorted order, depending on the value of the sorted parameter        
    if (!cwd->head) {                                 // if the cwd has no children (i.e., it is empty), print a message indicating that it is empty and return
        cout << "  (empty)\n"; 
        return; 
    }
    if (!sorted) {                                   // if the sorted parameter is false, print the contents of the cwd in unsorted order 
                                                     // by traversing the linked list of children and calling printNodeLine for each child
        for (Node* c = cwd->head; c; c = c->next){   // traverse the linked list of children of the cwd
             printNodeLine(c);                       // print the name and size of each child using the printNodeLine function
        }
    } 
    else {                                          // if the sorted parameter is true, create a binary search tree (BST) of the children of the cwd 
                                                    // and perform an inorder traversal to print the contents in sorted order   
        BSTNode* r = nullptr;                       // create a pointer to the root of the BST and initialize it to nullptr
        for (Node* c = cwd->head; c; c = c->next){  // traverse the linked list of children of the cwd and insert each child into the BST using the bstInsert function
             r = bstInsert(r, c);                   // insert the current child into the BST and update the root pointer to point to the new root of the BST
        }
        bstInorder(r);                              // perform an inorder traversal of the BST to print the names and sizes of the children in sorted order
        bstFree(r);                                 // free the memory used by the BST by calling the bstFree function
    }
}

void cmdCd(const string& path) {                   // this function changes the current working directory (cwd) to the folder specified by the given path
    Node* n = resolve(path);        
    if (!n){                                       // use the resolve function to find the Node corresponding to the given path, 
                                                   //and print an error message if it does not exist
       cout << "No such path.\n";
    }
    else if (!n->isFolder){                       // check if the Node is a folder, and print an error message if it is not
     cout << "Not a folder.\n";
    }
    else{                                        // if the Node is a folder, set the cwd to the new folder and print a message indicating that the cwd has been changed
        cout << "Changed folder to '" << getPath(n) << "'\n";
        cwd = n;
    }
}

void cmdRm(const string& path) {                // this function removes a file or folder specified by the given path from the file system
    Node* n = resolve(path);                    // use the resolve function to find the Node corresponding to the given path, and print an error message if it does not exist
    if (!n){                                    // if the Node does not exist, print an error message and return
        cout << "No such path.\n"; 
        return; 
    }
    if (n == root){                            // check if the Node is the root folder, and print an error message if it is
        cout << "Cannot remove root.\n"; 
        return; 
    }
    if (isInside(n, cwd)){                    // check if the Node is the current working directory (cwd) or a parent of the cwd, and print an error message if it is
        cout << "Cannot remove the folder you are inside.\n"; 
        return; 
    }
    Node* p = n->parent;                     // store the parent of the Node to be removed, so that it can be restored later if necessary
    disown(n);                               // remove the Node from its parent's linked list of children using the disown function                   
    undoStack.push({REMOVE, n, p});          // push an Action onto the undoStack to allow undoing the removal of the Node
    cout << "Removed (type 'undo' to restore).\n";
}

void cmdUndo() {                             // this function undoes the last action performed on the file system (either a creation or removal of a Node)
    if (undoStack.empty()){                  // check if the undoStack is empty, and print a message indicating that there is nothing to undo if it is
        cout << "Nothing to undo.\n"; 
        return; 
    }
    Action a = undoStack.top();              // get the last Action from the undoStack and pop it off the stack
    undoStack.pop();
    if (a.type == CREATE) {                 // if the Action was a creation of a Node, check if the Node is inside the 
                                            // current working directory (cwd) and update the cwd if necessary     
        if (isInside(a.node, cwd)) cwd = a.parent;  // if the Node is inside the cwd, set the cwd to the parent of the Node being undone 
        disown(a.node);                             // remove the Node from its parent's linked list of children using the disown function
        cout << "Undone: creation of '" << a.node->name << "'\n";
        destroy(a.node);                            // free the memory used by the Node being undone by calling the destroy function
    } else {                                    // if the Action was a removal of a Node, restore the Node to its parent's linked list of children using the adopt function     
        adopt(a.parent, a.node);
        cout << "Undone: removal of '" << a.node->name << "'\n";
    }
}

void printTree(Node* n, int depth) {                                 // this function prints the entire file system tree starting from the given Node, 
                                                                     // with indentation based on the depth of the Node in the tree
    for (int i = 0; i < depth; i++) cout << "  ";                    // print indentation based on the depth of the Node in the tree
    if (n->isFolder){                                                // if the Node is a folder, print "[D]" followed by the name of the folder
       cout << "[D] " << n->name << "\n";
    }
    else{                                                            // if the Node is a file, print "[F]" followed by the name of the file and its size in KB
        cout << "[F] " << n->name << " (" << n->size << " KB)\n";
    }
    for (Node* c = n->head; c; c = c->next){                         // recursively call printTree on each child of the Node, 
                                                                     // increasing the depth by 1 for each level of the tree
        printTree(c, depth + 1);
    }
}

int totalSize(Node* n) {                       // this function calculates the total size of a Node and all its children (if it is a folder) recursively
    if (!n->isFolder){                         // if the Node is a file, return its size
         return n->size;
    }
    int sum = 0;                              // if the Node is a folder, initialize a sum variable to 0 and 
                                              // recursively call totalSize on each child of the Node,
    for (Node* c = n->head; c; c = c->next){  // adding the size of each child to the sum variable
         sum += totalSize(c);
    }
    return sum;                               // return the total size of the Node and all its children
}


void cmdFind(const string& key) {                                // this function searches for all Nodes in the file system whose names contain the given key, 
                                                                 // starting from the root Node and using a breadth-first search (BFS) algorithm
    queue<Node*> q;                                              // create a queue to hold the Nodes to be searched, and push the root Node onto the queue
    q.push(root);
    int found = 0;                                               // initialize a counter variable to keep track of the number of matches found
    while (!q.empty()) {                                         // while the queue is not empty, pop the front Node off the queue and check if its name contains the key
        Node* cur = q.front();                                   // get the front Node from the queue
        q.pop();                                                 // pop the front Node off the queue
        if (cur != root && cur->name.find(key) != string::npos) {// if the Node is not the root and its name contains the key, 
                                                                 // print its path and increment the counter variable
            cout << "  " << getPath(cur) << "\n";                
            found++;                                             // increment the counter variable to keep track of the number of matches found
        }
        for (Node* c = cur->head; c; c = c->next){               // for each child of the current Node, push it onto the queue to be searched later
             q.push(c);
        }
    }
    if (!found) cout << "  No matches.\n";                      // if no matches were found, print a message indicating that there were no matches
}


void collectFiles(Node* n, priority_queue<pair<int, string>>& pq) { // this function collects all the files in the file system starting from the given Node, 
                                                                 // and adds them to a priority queue (max-heap) based on their sizes
    if (!n->isFolder) {                                          // if the Node is a file, push its size and path onto the priority queue
        pq.push({n->size, getPath(n)});                          
        return; 
    }
    for (Node* c = n->head; c; c = c->next){                    // for each child of the current Node, 
                                                                // recursively call collectFiles to collect all the files in its subtree
        collectFiles(c, pq);
    }
}

void cmdTop(int k) {                                     // this function prints the top k largest files in the file system, starting from the root Node
    if (k <= 0) {                                        // if k is less than or equal to 0, print an error message and return
        cout << "Give a positive number.\n"; 
        return; 
    }
    priority_queue<pair<int, string>> pq;               // create a priority queue (max-heap) to hold the sizes and paths of the files in the file system       
    collectFiles(root, pq);                             // call collectFiles to collect all the files in the file system 
                                                        // starting from the root Node and add them to the priority queue
    if (pq.empty()) {                                   // if the priority queue is empty (i.e., there are no files in the file system), 
                                                        // print a message indicating that there are no files and return
        cout << "  No files yet.\n"; 
        return; 
    }
    for (int i = 0; i < k && !pq.empty(); i++) {       // for each of the top k largest files, pop the front of the priority queue and print its size and path
        cout << "  " << pq.top().first << " KB  " << pq.top().second << "\n";
        pq.pop();                                     // pop the front of the priority queue to remove the largest file from the queue
    }
}

void printHelp() {                                   // this function prints a list of available commands and their descriptions to the console
    cout << "Commands:\n"
         << "  mkdir <name>         create a folder\n"
         << "  touch <name> [KB]    create a file (optional size in KB)\n"
         << "  ls [-s]              list current folder (-s = sorted A-Z)\n"
         << "  cd <path>            change folder (cd alone = root)\n"
         << "  pwd                  print current path\n"
         << "  rm <path>            remove file/folder\n"
         << "  tree                 show whole structure\n"
         << "  find <text>          search names (BFS)\n"
         << "  du [path]            total size of a file/folder\n"
         << "  top <n>              n largest files\n"
         << "  undo                 undo last mkdir/touch/rm\n"
         << "  history              commands used so far\n"
         << "  help, exit\n";
}


int main() {                                       // this is the main function that initializes the file system simulation and handles user input
    root = new Node("/", true);                    // create a new Node for the root folder with the name "/" and set it as a folder
    cwd = root;                                    // set the current working directory (cwd) to the root folder
    vector<string> history;                        // create a vector to hold the history of commands entered by the user     
    cout << "File System Simulation. Type 'help' for commands.\n";

    string line;                                   // create a string to hold the user's input line
    while (true) {                                 // loop indefinitely to handle user input until the user exits the program
        cout << getPath(cwd) << " > ";             // print the current path followed by a prompt symbol to indicate that the program is ready for user input
        if (!getline(cin, line)) break;            // read a line of input from the user and break the loop if there is an error or end-of-file

       
        vector<string> t;                         // create a vector to hold the tokens (words) in the user's input line
        { 
            istringstream iss(line);              // create an input string stream from the user's input line to tokenize it into words
            string w;                             // create a string to hold each word as it is extracted from the input stream
            while (iss >> w) t.push_back(w);      // extract each word from the input stream and push it onto the vector of tokens
        }
        if (t.empty()) continue;                  // if the vector of tokens is empty (i.e., the user entered a blank line), 
                                                  // continue to the next iteration of the loop to prompt for input again
        history.push_back(line);                  // add the user's input line to the history vector to keep track of the commands entered so far

        const string& c = t[0];                   // create a reference to the first token (the command) in the vector of tokens for easier access
        if (c == "exit") break;                   // if the command is "exit", break the loop to exit the program
        else if (c == "help") printHelp();        // if the command is "help", call the printHelp function to display a list of available commands and their descriptions
        else if (c == "mkdir" && t.size() == 2){  // if the command is "mkdir" and there are exactly two tokens (the command and the folder name), 
                                                  // call the cmdCreate function to create a new folder with the specified name
            cmdCreate(t[1], true, 0);
        }
        else if (c == "touch" && (t.size() == 2 || t.size() == 3)) { // if the command is "touch" and there are either two or three tokens 
                                                                     // (the command, the file name, and an optional size),
            int size = 1;                                            // set the default size of the file to 1 KB if no size is specified
            if (t.size() == 3) {                                     // if a size is specified, attempt to convert it to an integer using stoi 
                                                                     // and catch any exceptions that may occur (e.g., if the input is not a valid number)
                try { size = stoi(t[2]); } catch (...) { size = -1; } // if an exception occurs, set the size to -1 to indicate an invalid size
            }
            if (size < 0) cout << "Size must be a non-negative number.\n"; // if the size is negative, print an error message indicating that the size must be a non-negative number
            else cmdCreate(t[1], false, size);                       // if the size is valid, call the cmdCreate function to create 
                                                                     // a new file with the specified name and size
        }
        else if (c == "ls") cmdLs(t.size() == 2 && t[1] == "-s");    // if the command is "ls", call the cmdLs function to list the contents of the current working directory (cwd) 
                                                                     // in either sorted or unsorted order, depending on whether the "-s" option is specified
        else if (c == "cd") cmdCd(t.size() >= 2 ? t[1] : "/");       // if the command is "cd", call the cmdCd function to change the cwd to the specified folder, 
                                                                     // or to the root folder if no folder is specified
        else if (c == "pwd") cout << getPath(cwd) << "\n";           // if the command is "pwd", print the current path of the cwd using the getPath function
        else if (c == "rm" && t.size() == 2) cmdRm(t[1]);            // if the command is "rm" and there are exactly two tokens (the command and the path), 
                                                                     // call the cmdRm function to remove the specified file or folder from the file system
        else if (c == "tree") printTree(root, 0);                    // if the command is "tree", call the printTree function to print the entire file system tree starting from the root Node, 
                                                                     // with indentation based on the depth of each Node in the tree
        else if (c == "find" && t.size() == 2) cmdFind(t[1]);        // if the command is "find" and there are exactly two tokens (the command and the search key), 
                                                                     // call the cmdFind function to search for all Nodes in the file system whose names contain the specified key
        else if (c == "du") {                                        // if the command is "du", call the totalSize function to calculate the total size of the specified file or folder, 
                                                                     // or the current working directory (cwd) if no path is specified
            Node* n = resolve(t.size() >= 2 ? t[1] : ".");           // use the resolve function to find the Node corresponding to the specified path, or the cwd if no path is specified
            if (n) cout << "  " << totalSize(n) << " KB\n";          // if the Node exists, print its total size in KB using the totalSize function
            else   cout << "No such path.\n";                        // if the Node does not exist, print an error message indicating that there is no such path
        }
        else if (c == "top" && t.size() == 2) {                      // if the command is "top" and there are exactly two tokens (the command and the number of files to display), 
                                                                     // attempt to convert the second token to an integer using stoi and call the cmdTop function to print the top n largest files
            try { cmdTop(stoi(t[1])); } catch (...) { cout << "Give a number.\n"; }
        }
        else if (c == "undo") cmdUndo();                             // if the command is "undo", call the cmdUndo function to undo the 
                                                                     // last action performed on the file system (either a creation or removal of a Node)
        else if (c == "history") {                                   // if the command is "history", print the history of commands entered by the user so far, 
                                                                     // with each command preceded by its index in the history vector
            for (size_t i = 0; i < history.size(); i++)
                cout << "  " << i + 1 << "  " << history[i] << "\n";
        }
        else cout << "Unknown or incomplete command. Type 'help'.\n";
    }


    destroy(root);                             // free the memory used by the file system tree starting from the root Node by calling the destroy function
    while (!undoStack.empty()) {               // while the undoStack is not empty, pop each Action off the stack and free the 
                                               // memory used by any Nodes that were created but not undone
        if (undoStack.top().type == REMOVE){   // if the Action was a removal of a Node, free the memory used by the Node that was removed by calling the destroy function
            destroy(undoStack.top().node);     // free the memory used by the Node that was removed by calling the destroy function
        }
        undoStack.pop();                      // pop the Action off the undoStack to remove it from the stack
    }
    return 0;
}