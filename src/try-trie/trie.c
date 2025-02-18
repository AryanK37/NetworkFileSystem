#include "trie.h"

int is_file(trienode *root, char *str)
{
    for (int i = 0; i < (int)strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return -1; // invalid path
        }
        root = root->child[(unsigned char)str[i]];
    }
    if (root->child[(int)'/'] == NULL)
    {
        if (root->hashind != -1)
        {             // there is a server for this file so it is a file
            return 1; // is file
        }
        return -1; // is not a file
    }
    return 0; // is directory
}

void print_all_subtree_complete(trienode *node, char *path, int level, FILE *fp, char *str)
{
    if (node == NULL)
    {
        return;
    }

    // if (node->hashind != -1)
    int printed = 0;
    {
        path[level] = '\0';
        int cnt = 0;
        for (int i = 0; i < 256; i++)
        {
            if (node->child[i] != NULL)
            {
                cnt++;
                break;
            }
        }
        if (!cnt)
        {
            fprintf(fp, "%s", str);
            fprintf(fp, "%s\n", path);
            printed = 1;
        }
    }

    if (!printed && node->hashind != -1)
    {
        fprintf(fp, "%s", str);
        fprintf(fp, "%s\n", path);
    }

    for (int i = 0; i < 256; i++)
    {
        // char c = (char)i;
        if (node->child[i] != NULL)
        {
            path[level] = (char)i; // Append the character to the current path
            print_all_subtree_complete(node->child[i], path, level + 1, fp, str);
        }
    }
}

int print_all_childs_v2(trienode *root, char *str, FILE *fp)
{
    int len = (int)strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return 0;
        }
        root = root->child[(unsigned char)str[i]];
    }
    char path[256];

    print_all_subtree_complete(root, path, 0, fp, str);
    return 1;
}

int find_subtree_new(trienode *node) // returns what server it is in
{
    if (node == NULL)
    {
        return -1;
    }
    if (node->hashind != -1)
    {
        return node->hashind;
    }
    for (int i = 0; i < 256; i++)
    {
        if (node->child[i] != NULL)
        {
            int y = find_subtree_new(node->child[i]);
            if (y != -1)
            {
                return y;
            }
        }
    }
    return -1;
}

int find_new(trienode *root, char *str, int *is_partial)
{
    if (root == NULL)
    {
        return -1;
    }
    int i = 0;
    trienode *lastslash = root;
    trienode *rootc = root;
    *is_partial = 1;
    for (; i < (int)strlen(str); i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            *is_partial = 1;
            break;
        }
        if (str[i] == '/')
        {
            lastslash = root;
        }
        root = root->child[(unsigned char)str[i]];
    }

    if (i == (int)strlen(str))
    {
        // is file, directory or incomplete match
        // *is_partial = 0;
        if (root->hashind != -1 || root->child[(int)'/'] != NULL)
        {
            *is_partial = 0;
            // return root->hashind;
        }
        else
        {
            *is_partial = 1;
        }

        return find_subtree_new(root);
    }
    *is_partial = 1;
    if (lastslash != rootc)
    {
        return find_subtree_new(lastslash);
    }
    return -1;
}
trienode *newnode()
{
    trienode *node = (trienode *)malloc(sizeof(trienode));
    node->lastnode = false;
    if (pthread_mutex_init(&node->lock, NULL) != 0)
    {
        return NULL;
    }
    node->hashind = -1;
    for (int i = 0; i < 256; i++)
    {
        node->child[i] = NULL;
    }
    return node;
}

int trieinsert(trienode *temp, char *str, int hashind)
{
    for (int i = 0; i < (int)strlen(str); i++)
    {
        if (temp->child[(unsigned char)str[i]] == NULL)
        {
            temp->child[(unsigned char)str[i]] = newnode();
        }
        temp = temp->child[(unsigned char)str[i]];
    }
    if (temp->hashind != -1)
    {
        return -1; // Duplicate entry
    }
    else
    {
        temp->hashind = hashind;
        temp->lastnode = true;
        return temp->hashind;
    }
}

void initialize_trie(trienode **root)
{
    *root = newnode();
    return;
}

int delrec2(trienode *parent, trienode *child, int i, char *str, int assn)
{
    if (child == NULL)
    {
        return 0;
    }

    if (str[i] == '\0')
    {
        // int hasChildren = 0;
        // for (int k = 0; k < 256; k++) {
        //     if (k==(int)'/')
        //     {
        //         continue;
        //     }
        //     if (child->child[k] != NULL) {
        //         hasChildren = 1;
        //         break;
        //     }
        // }
        // if (hasChildren) {
        //     return 1;
        // }
        if (parent)
        {
            parent->child[(unsigned int)str[i - 1]] = NULL;
            parent->hashind = assn;
        }
        // free(child);
        return 1;
    }

    int nextIndex = (unsigned int)str[i];
    int x = delrec2(child, child->child[nextIndex], i + 1, str, assn);

    if (x)
    {
        int hasChildren = 0;
        for (int k = 0; k < 256; k++)
        {
            // if (k==(int)str[i])
            // {
            //     continue;
            // }

            if (child->child[k] != NULL)
            {
                hasChildren++;
                // break;
            }
        }
        if (hasChildren <= 0)
        {
            // free(child);
            // if (parent) {
            //     parent->child[nextIndex] = NULL;
            //     parent->hashind = assn;
            // }
            if (parent)
            {
                parent->child[(unsigned int)str[i - 1]] = NULL;
                parent->hashind = assn;
            }
            return 1;
        }
    }

    return 0;
}

int delete_from_trie(trienode *root, char *str, int assn)
{
    if (root == NULL)
    {
        return -1;
    }

    return delrec2(NULL, root->child[(unsigned int)str[0]], 1, str, assn);
}
void print_all_subtree(trienode *node, char *path, int level, FILE *fp)
{
    if (node == NULL)
    {
        return;
    }

    if (node->hashind != -1 || node->child[(int)'/'] != NULL)
    {
        path[level] = '\0';
        fprintf(fp, "%s\n", path);
    }

    for (int i = 0; i < 256; i++)
    {
        char c = (char)i;
        if (c == '/')
        {
            continue;
        }
        if (node->child[i] != NULL)
        {
            path[level] = (char)i; // Append the character to the current path
            print_all_subtree(node->child[i], path, level + 1, fp);
        }
    }
}

int print_all_childs(trienode *root, char *str, FILE *fp)
{
    int len = (int)strlen(str);

    for (int i = 0; i < len; i++)
    {
        if (root->child[(unsigned char)str[i]] == NULL)
        {
            return 0;
        }
        root = root->child[(unsigned char)str[i]];
    }
    char path[256];
    if (root->child[(int)'/'] == NULL)
    {
        if (root->hashind != -1)
        {
            fprintf(fp, "%s\n", str);
        }
        else
        {
            return 0;
        }
        return 1;
    }
    root = root->child[(int)'/'];

    print_all_subtree(root, path, 0, fp);
    return 1;
}
