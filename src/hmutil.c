/**
 *  @file hmutil.c
 *  @brief HM Utility functions (AVL Tree functions)
 *
 *  @author Anshul
 *  @date 30-Jul-2015
 *  @bug None
 */

/***************************************************************************/
/* AVL Tree Functions                             */
/***************************************************************************/
#include <hmincl.h>

/**
 *  @brief Insert the supplied node into the specified AVL3 tree if key does not already exist,
 *      otherwise returning the existing node
 *
 *  Scan down the tree looking for the insert point, going left if the insert key
 *  is less than the key in the tree and right if it is greater.
 *  When the insert point is found insert the new node and rebalance the tree if necessary.
 *  Return the existing entry instead, if found.
 *
 *  @param *tree a pointer to the AVL3 tree
 *  @param *node a pointer to the node to insert
 *  @param *tree_info a pointer to the AVL3 tree info
 *
 *  @return Pointer to the tree node if previous entry exists, @c NULL otherwise
 */
VOID *avl3_insert_or_find(HM_AVL3_TREE *tree,
                                  HM_AVL3_NODE *node,
                                  const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* insert specified node into tree                                         */
  /***************************************************************************/
  HM_AVL3_NODE *parent_node;
  int32_t result;
  VOID *existing_entry = NULL;

  TRACE_ENTRY();

  TRACE_ASSERT(HM_AVL3_IN_TREE(*node) == FALSE);

  TRACE_INFO(("Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  node->right_height = 0;
  node->left_height = 0;

  if (tree->root == NULL)
  {
    /*************************************************************************/
    /* tree is empty, so insert at root                                      */
    /*************************************************************************/
    TRACE_INFO(("tree is empty, so insert at root" ));
    tree->root = node;
    tree->first = node;
    tree->last = node;
    goto EXIT_LABEL;
  }

  /***************************************************************************/
  /* scan down the tree looking for the appropriate insert point             */
  /***************************************************************************/
  TRACE_INFO(("scan for insert point" ));
  parent_node = tree->root;
  while (parent_node != NULL)
  {
    /*************************************************************************/
    /* go left or right, depending on comparison                             */
    /*************************************************************************/
    TRACE_INFO(("parent %p", parent_node));
    result = tree_info->compare((VOID *)((BYTE *)node -
                                             tree_info->node_offset +
                                             tree_info->key_offset),

                (VOID *)((BYTE *)parent_node -
                                             tree_info->node_offset +
                                             tree_info->key_offset)
                );

    if (result > 0)
    {
      /***********************************************************************/
      /* new key is greater than this node's key, so move down right subtree */
      /***********************************************************************/
      TRACE_INFO(("move down right subtree" ));
      if (parent_node->right == NULL)
      {
        /*********************************************************************/
        /* right subtree is empty, so insert here                            */
        /*********************************************************************/
        TRACE_INFO(("right subtree empty, insert here" ));
        TRACE_ASSERT(node != parent_node);
        node->parent = parent_node;
        parent_node->right = node;
        parent_node->right_height = 1;
        if (parent_node == tree->last)
        {
          /*******************************************************************/
          /* parent was the right-most node in the tree, so new node is now  */
          /* right-most                                                      */
          /*******************************************************************/
          TRACE_INFO(("new last node" ));
          tree->last = node;
        }
        break;
      }
      else
      {
        /*********************************************************************/
        /* right subtree is not empty                                        */
        /*********************************************************************/
        TRACE_INFO(("right subtree not empty" ));
        parent_node = parent_node->right;
      }
    }
    else if (result < 0)
    {
      /***********************************************************************/
      /* new key is less than this node's key, so move down left subtree     */
      /***********************************************************************/
      TRACE_INFO(("move down left subtree" ));
      if (parent_node->left == NULL)
      {
        /*********************************************************************/
        /* left subtree is empty, so insert here                             */
        /*********************************************************************/
        TRACE_INFO(("left subtree empty, insert here" ));
        TRACE_ASSERT(node != parent_node);
        node->parent = parent_node;
        parent_node->left = node;
        parent_node->left_height = 1;
        if (parent_node == tree->first)
        {
          /*******************************************************************/
          /* parent was the left-most node in the tree, so new node is now   */
          /* left-most                                                       */
          /*******************************************************************/
          TRACE_INFO(("new first node" ));
          tree->first = node;
        }
        break;
      }
      else
      {
        /*********************************************************************/
        /* left subtree is not empty                                         */
        /*********************************************************************/
        TRACE_INFO(("left subtree not empty" ));
        parent_node = parent_node->left;
      }
    }
    else
    {
      /***********************************************************************/
      /* found a matching key, so get out now and return entry found         */
      /***********************************************************************/
      TRACE_INFO(("found matching key" ));

      existing_entry = (VOID *)
                            ((BYTE *)parent_node - tree_info->node_offset);
      node->right_height = -1;
      node->left_height = -1;
      goto EXIT_LABEL;
    }
  }

  /***************************************************************************/
  /* now rebalance the tree if necessary                                     */
  /***************************************************************************/
  TRACE_INFO(("rebalance the tree" ));
  avl3_balance_tree(tree, parent_node);

EXIT_LABEL:

  TRACE_EXIT();

  return(existing_entry);

} /* avl3_insert_or_find */

/**
 *  @brief Delete the specified node from the specified AVL3 tree
 *
 *  @param *tree A pointer to the AVL3 Tree
 *  @param *node a pointer to the node to delete
 *
 *  @return @c void
 */
VOID avl3_delete(HM_AVL3_TREE *tree, HM_AVL3_NODE *node)
{
  /***************************************************************************/
  /* delete specified node from tree                                         */
  /***************************************************************************/
  HM_AVL3_NODE *replace_node;
  HM_AVL3_NODE *parent_node;
  uint16_t new_height;
#ifdef I_WANT_TO_DEBUG
  HM_AVL3_NODE *check_node;
#endif

  TRACE_ENTRY();

  TRACE_ASSERT(HM_AVL3_IN_TREE(*node) != FALSE);

  /***************************************************************************/
  /* Assert that the specified node is in the specified tree.  This is done  */
  /* by following the parent pointers from the specified node to the root    */
  /* node then asserting that this is the root node for the specified tree.  */
  /* Assert failure indicates one of the following:                          */
  /* -  The caller is attempting to delete a node from a tree rooted in a    */
  /*    control block that has been freed.                                   */
  /*    Without the assert this risks memory corruption.                     */
  /* -  The caller is attempting to delete a node from the wrong tree.       */
  /*    Without the assert this would silently corrupt one or both trees.    */
  /* -  The tree is already corrupt.                                         */
  /***************************************************************************/
#ifdef I_WANT_TO_DEBUG
  check_node = node;
  while (check_node->parent != NULL)
  {
    TRACE_INFO(("parent = %p", check_node->parent));
    check_node = check_node->parent;
  }
  TRACE_ASSERT(tree->root == check_node);
#endif

  if ((node->left == NULL) &&
      (node->right == NULL))
  {
    /*************************************************************************/
    /* barren node (no children), so just delete it                          */
    /*************************************************************************/
    TRACE_INFO(("delete barren node" ));
    replace_node = NULL;

    if (tree->first == node)
    {
      /***********************************************************************/
      /* node was first in tree, so replace it                               */
      /* Left-most element (lightest)                     */
      /***********************************************************************/
      TRACE_INFO(("replace first node in tree" ));
      tree->first = node->parent;
    }

    if (tree->last == node)
    {
      /***********************************************************************/
      /* node was last in tree, so replace it                                */
      /* Right-most element (heaviest)                     */
      /***********************************************************************/
      TRACE_INFO(("replace last node in tree" ));
      tree->last = node->parent;
    }
  }
  else if (node->left == NULL)
  {
    /*************************************************************************/
    /* node has no left son, so replace with right son                       */
    /*************************************************************************/
    TRACE_INFO(("node has no left son, replace with right son" ));
    replace_node = node->right;

    if (tree->first == node)
    {
      /***********************************************************************/
      /* node was first in tree, so replace it                               */
      /***********************************************************************/
      TRACE_INFO(("replace first node in tree" ));
      tree->first = replace_node;
    }
  }
  else if (node->right == NULL)
  {
    /*************************************************************************/
    /* node has no right son, so replace with left son                       */
    /*************************************************************************/
    TRACE_INFO(("node has no right son, replace with left son" ));
    replace_node = node->left;

    if (tree->last == node)
    {
      /***********************************************************************/
      /* node was last in tree, so replace it                                */
      /***********************************************************************/
      TRACE_INFO(("replace last node in tree" ));
      tree->last = replace_node;
    }
  }
  else
  {
    /*************************************************************************/
    /* node has both left and right-sons                                     */
    /*************************************************************************/
    TRACE_INFO(("node has two sons" ));
    TRACE_ASSERT(node->left != node->right);
    if (node->right_height > node->left_height)
    {
      /***********************************************************************/
      /* right subtree is higher than left subtree                           */
      /***********************************************************************/
      TRACE_INFO(("right subtree is higher" ));
      if (node->right->left == NULL)
      {
        /*********************************************************************/
        /* can replace node with right-son (since it has no left-son)        */
        /*********************************************************************/
        TRACE_INFO(("replace node with right son" ));
        replace_node = node->right;
        replace_node->left = node->left;
        TRACE_ASSERT(replace_node->left != replace_node);
        replace_node->left->parent = replace_node;
        replace_node->left_height = node->left_height;
      }
      else
      {
        /*********************************************************************/
        /* swap with left-most descendent of right subtree                   */
        /*********************************************************************/
        TRACE_INFO(("swap with left-most right descendent" ));
        avl3_swap_left_most(tree, node->right, node);
        replace_node = node->right;
      }
    }
    else
    {
      /***********************************************************************/
      /* left subtree is higher (or subtrees are of same height)             */
      /***********************************************************************/
      TRACE_INFO(("left subtree is higher" ));
      TRACE_INFO(("(or both subtrees are of equal height)" ));
      if (node->left->right == NULL)
      {
        /*********************************************************************/
        /* can replace node with left-son (since it has no right-son)        */
        /*********************************************************************/
        TRACE_INFO(("replace node with left son" ));
        replace_node = node->left;
        replace_node->right = node->right;
        TRACE_ASSERT(replace_node->right != replace_node);
        replace_node->right->parent = replace_node;
        replace_node->right_height = node->right_height;
      }
      else
      {
        /*********************************************************************/
        /* swap with right-most descendent of left subtree                   */
        /*********************************************************************/
        TRACE_INFO(("swap with right-most left descendent" ));
        avl3_swap_right_most(tree, node->left, node);
        replace_node = node->left;
      }
    }
  }

  /***************************************************************************/
  /* save parent node of deleted node                                        */
  /***************************************************************************/
  parent_node = node->parent;

  /***************************************************************************/
  /* reset deleted node                                                      */
  /***************************************************************************/
  TRACE_INFO(("reset deleted node" ));
  node->parent = NULL;
  node->right = NULL;
  node->left = NULL;
  node->right_height = -1;
  node->left_height = -1;

  if (replace_node != NULL)
  {
    /*************************************************************************/
    /* fix-up parent pointer of replacement node, and calculate new height   */
    /* of subtree                                                            */
    /*************************************************************************/
    TRACE_INFO(("fixup parent pointer of replacement node" ));
    TRACE_ASSERT(replace_node != parent_node);
    replace_node->parent = parent_node;
    new_height = (uint16_t)
                 (1 + MAX(replace_node->left_height, replace_node->right_height));
  }
  else
  {
    /*************************************************************************/
    /* no replacement, so new height of subtree is zero                      */
    /*************************************************************************/
    TRACE_INFO(("new height zero"));
    new_height = 0;
  }
  TRACE_INFO(("new height of parent is %d", new_height ));

  if (parent_node != NULL)
  {
    /*************************************************************************/
    /* fixup parent node                                                     */
    /*************************************************************************/
    TRACE_INFO(("fix-up parent node" ));
    if (parent_node->right == node)
    {
      /***********************************************************************/
      /* node is right son of parent                                         */
      /***********************************************************************/
      TRACE_INFO(("replacement node is right son" ));
      parent_node->right = replace_node;
      parent_node->right_height = new_height;
    }
    else
    {
      /***********************************************************************/
      /* node is left son of parent                                          */
      /***********************************************************************/
      TRACE_INFO(("replacement node is left son" ));
      parent_node->left = replace_node;
      parent_node->left_height = new_height;
    }

    /*************************************************************************/
    /* now rebalance the tree (if necessary)                                 */
    /*************************************************************************/
    TRACE_INFO(("rebalance the tree" ));
    avl3_balance_tree(tree, parent_node);
  }
  else
  {
    /*************************************************************************/
    /* replacement node is now root of tree                                  */
    /*************************************************************************/
    TRACE_INFO(("replacement node is now root of tree" ));
    tree->root = replace_node;
  }

  TRACE_EXIT();

  return;

} /* avl3_delete */

/**
 *  @brief Find the node in the AVL3 tree with the supplied key
 *
 *  Search down the tree going left if the search key is less than
 *  the node in the tree and right if the search key is greater.
 *  When we run out of tree to search through either we've found
 *  it or the node is not in the tree.
 *
 *  @param *tree A pointer to the AVL3 tree
 *  @param *key A pointer to the key to compare and find
 *  @param *tree_info A pointer to AVL3 tree info
 *
 *  @return A pointer to the node. NULL if no node is found with the specified key
 */
VOID *avl3_find(HM_AVL3_TREE *tree,
        VOID *key,
        const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* find node with specified key                                            */
  /***************************************************************************/
  HM_AVL3_NODE *node;
  int32_t result;

  TRACE_ENTRY();

  TRACE_INFO(("Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  node = tree->root;
#ifdef I_WANT_TO_DEBUG
  if(node== NULL)
  {
    TRACE_DETAIL(("Tree is empty!"));
  }
#endif
  while (node != NULL)
  {
    /*************************************************************************/
    /* compare key of current node with supplied key                         */
    /*************************************************************************/
    TRACE_INFO(("node %p", node));
    result = tree_info->compare(key,
                                (VOID *)((BYTE *)node -
                                             tree_info->node_offset +
                                             tree_info->key_offset)
                 );

    if (result > 0)
    {
      /***********************************************************************/
      /* specified key is greater than key of this node, so look in right    */
      /* subtree                                                             */
      /***********************************************************************/
      TRACE_INFO(( "move down right subtree" ));
      node = node->right;
    }
    else if (result < 0)
    {
      /***********************************************************************/
      /* specified key is less than key of this node, so look in left        */
      /* subtree                                                             */
      /***********************************************************************/
      TRACE_INFO(("move down left subtree" ));
      node = node->left;
    }
    else
    {
      /***********************************************************************/
      /* found the requested node                                            */
      /***********************************************************************/
      TRACE_INFO(("found requested node" ));
      break;
    }
  }

  TRACE_EXIT();

  return((node != NULL) ?
               (VOID *)((BYTE *)node - tree_info->node_offset) : NULL);

} /* avl3_find */

/**
 *  @brief Find the first entry in the AVL3 tree.
 *
 *  @param *tree a pointer to the AVL3 tree
 *  @param *tree_info a pointer to the AVL3 tree info
 *
 *  @return A pointer to the first entry.  @c NULL if the tree is empty.
 */
VOID *avl3_first(HM_AVL3_TREE *tree,
         const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* find first node in tree                                                 */
  /***************************************************************************/
  HM_AVL3_NODE *node;

  TRACE_ENTRY();

  TRACE_INFO(("Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  if (tree->first != NULL)
  {
    TRACE_INFO(("Tree not empty"));
    node = tree->first;
  }
  else
  {
    TRACE_INFO(("Tree empty"));
    node = NULL;
  }

  TRACE_EXIT();

  return((node != NULL) ?
               (VOID *)((BYTE *)node - tree_info->node_offset) : NULL);

} /* avl3_first */

/**
 *  @brief Find the last entry in the AVL3 tree.
 *
 *  @param *tree A pointer to the AVL3 Tree
 *  @param *tree_info a pointer to the AVL3 tree info
 *
 *  @return A pointer to the last entry.  NULL if the tree is empty.
 */
VOID *avl3_last(HM_AVL3_TREE *tree,
                        const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* find last node in tree                                                  */
  /***************************************************************************/
  HM_AVL3_NODE *node;

  TRACE_ENTRY();

  TRACE_INFO(("Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  if (tree->last != NULL)
  {
    TRACE_INFO(("Tree not empty"));
    node = tree->last;
  }
  else
  {
    TRACE_INFO(("Tree empty"));
    node = NULL;
  }

  TRACE_EXIT();

  return((node != NULL) ?
               (VOID *)((BYTE *)node - tree_info->node_offset) : NULL);

} /* avl3_last */

/**
 *  @brief Find next node in the AVL3 tree
 *
 *  If the specified node has a right-son then return the left-most son of this.
 *  Otherwise search back up until we find a node of which we are in the
 *  left sub-tree and return that.
 *
 *  @param *node a pointer to the current node in the tree
 *  @param *tree_info a pointer to the AVL3 tree info
 *
 *  @return A pointer to the next node in the tree
 */
VOID *avl3_next(HM_AVL3_NODE *node,
          const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* find next node in tree                                                  */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_INFO(("Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  TRACE_ASSERT(HM_AVL3_IN_TREE(*node) == TRUE);

  if (node->right != NULL)
  {
    /*************************************************************************/
    /* next node is left-most node in right subtree                          */
    /*************************************************************************/
    TRACE_INFO(("next node is left-most right descendent" ));
    node = node->right;
    while (node->left != NULL)
    {
      /***********************************************************************/
      /* FLOW TRACING NOT REQUIRED    Reason: Tight loop.                    */
      /***********************************************************************/
      node = node->left;
    }
  }
  else
  {
    /*************************************************************************/
    /* no right-son, so find a node of which we are in the left subtree      */
    /*************************************************************************/
    TRACE_INFO(("find node which this is in left subtree of" ));

    while (node != NULL)
    {
      /***********************************************************************/
      /* FLOW TRACING NOT REQUIRED    Reason: Tight loop.                    */
      /***********************************************************************/
      if ((node->parent == NULL) ||
          (node->parent->left == node))
      {
        TRACE_INFO(("found node %p", node));
        node = node->parent;
        break;
      }
      node = node->parent;
    }
  }

  TRACE_EXIT();

  return((node != NULL) ?
               (VOID *)((BYTE *)node - tree_info->node_offset) : NULL);

} /* avl3_next */


/**
 *  @brief Find previous node in the AVL3 tree
 *
 * If we have a left-son then the previous node is the right-most
 * son of this.  Otherwise, look for a node of whom we are in the
 * left subtree and return that.
 *
 *  @param *node a pointer to the current node in the tree
 *  @param *tree_info a pointer to the AVL3 tree info
 *
 *  @return A pointer to the previous node in the tree
 */
VOID *avl3_prev(HM_AVL3_NODE *node,
                        const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* find previous node in tree                                              */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_INFO(("Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  TRACE_ASSERT(HM_AVL3_IN_TREE(*node) == TRUE);

  if (node->left != NULL)
  {
    /*************************************************************************/
    /* previous node is right-most node in left subtree                      */
    /*************************************************************************/
    TRACE_INFO(("previous node is right-most left descendent" ));
    node = node->left;
    while (node->right != NULL)
    {
      /***********************************************************************/
      /* FLOW TRACING NOT REQUIRED    Reason: Tight loop.                    */
      /***********************************************************************/
      node = node->right;
    }
  }
  else
  {
    /*************************************************************************/
    /* no left-son, so find a node of which we are in the right subtree      */
    /*************************************************************************/
    TRACE_INFO(("find node which this is in right subtree of"));
    while (node != NULL)
    {
      /***********************************************************************/
      /* FLOW TRACING NOT REQUIRED    Reason: Tight loop.                    */
      /***********************************************************************/
      if ((node->parent == NULL) ||
          (node->parent->right == node))
      {
        TRACE_INFO(("found node %p", node));
        node = node->parent;
        break;
      }
      node = node->parent;
    }
  }

  TRACE_EXIT();

  return((node != NULL) ?
               (VOID *)((BYTE *)node - tree_info->node_offset) : NULL);

} /* avl3_prev */

/**
 *  @brief Rebalance the tree starting at the supplied node and ending at the root of the tree
 *
 *  @param *tree   a pointer to the AVL3 tree
 *  @param *node   a pointer to the node to start balancing from
 *  @return None
 */
VOID avl3_balance_tree(HM_AVL3_TREE *tree,
                               HM_AVL3_NODE *node)
{
  /***************************************************************************/
  /* balance the tree starting at the supplied node, and ending at the root  */
  /* of the tree                                                             */
  /***************************************************************************/
  TRACE_ENTRY();

  while (node->parent != NULL)
  {
    /*************************************************************************/
    /* node has uneven balance, so may need to rebalance it                  */
    /*************************************************************************/
    TRACE_INFO(("check node balance" ));
    TRACE_INFO(("  r_height = %d", node->right_height ));
    TRACE_INFO(("  l_height = %d", node->left_height ));

    if (node->parent->right == node)
    {
      /***********************************************************************/
      /* node is right-son of its parent                                     */
      /***********************************************************************/
      TRACE_INFO(("node is right-son" ));
      node = node->parent;
      avl3_rebalance(&node->right);

      /***********************************************************************/
      /* now update the right height of the parent                           */
      /***********************************************************************/
      node->right_height = (uint16_t)
                   (1 + MAX(node->right->right_height, node->right->left_height));
      TRACE_INFO(("new r_height = %d", node->right_height ));
    }
    else
    {
      /***********************************************************************/
      /* node is left-son of its parent                                      */
      /***********************************************************************/
      TRACE_INFO(("node is left-son" ));
      node = node->parent;
      avl3_rebalance(&node->left);

      /***********************************************************************/
      /* now update the left height of the parent                            */
      /***********************************************************************/
      node->left_height = (uint16_t)
                     (1 + MAX(node->left->right_height, node->left->left_height));
      TRACE_INFO(("new l_height = %d", node->left_height ));
    }
  }

  if (node->left_height != node->right_height)
  {
    /*************************************************************************/
    /* rebalance root node                                                   */
    /*************************************************************************/
    TRACE_INFO(("rebalance root node" ));
    avl3_rebalance(&tree->root);
    TRACE_ASSERT(tree->root != NULL);
  }

  TRACE_EXIT();

  return;

} /* avl3_balance_tree */

/**
 *  @brief Rebalance a subtree of the AVL3 tree (if necessary)
 *
 *  @param **subtree a pointer to the subtree to rebalance
 *  @return Nothing
 */
VOID avl3_rebalance(HM_AVL3_NODE **subtree)
{
  /***************************************************************************/
  /* Local data                                                              */
  /***************************************************************************/
  int32_t moment;

  /***************************************************************************/
  /* rebalance a subtree of the AVL3 tree                                    */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_INFO(("rebalance subtree" ));
  TRACE_INFO(("  r_height = %d", (*subtree)->right_height ));
  TRACE_INFO(("  l_height = %d", (*subtree)->left_height ));

  /***************************************************************************/
  /* How unbalanced - don't want to recalculate                              */
  /***************************************************************************/
  moment = (*subtree)->right_height - (*subtree)->left_height;

  if (moment > 1)
  {
    /*************************************************************************/
    /* subtree is heavy on the right side                                    */
    /*************************************************************************/
    TRACE_INFO(("subtree is heavy on right side" ));
    TRACE_INFO(("right subtree" ));
    TRACE_INFO(("  r_height = %d",
                                                 (*subtree)->right->right_height));
    TRACE_INFO(("  l_height = %d",
                                                 (*subtree)->right->left_height));
    if ((*subtree)->right->left_height > (*subtree)->right->right_height)
    {
      /***********************************************************************/
      /* right subtree is heavier on left side, so must perform right        */
      /* rotation on this subtree to make it heavier on the right side       */
      /***********************************************************************/
      TRACE_INFO(("right subtree is heavier on left side ..." ));
      TRACE_INFO(("... so rotate it right" ));
      avl3_rotate_right(&(*subtree)->right );
      TRACE_INFO(( "right subtree" ));
      TRACE_INFO(( "  r_height = %d",
                                                 (*subtree)->right->right_height));
      TRACE_INFO(( "  l_height = %d",
                                                 (*subtree)->right->left_height));
    }

    /*************************************************************************/
    /* now rotate the subtree left                                           */
    /*************************************************************************/
    TRACE_INFO(( "rotate subtree left" ));
    avl3_rotate_left(subtree );
  }
  else if (moment < -1)
  {
    /*************************************************************************/
    /* subtree is heavy on the left side                                     */
    /*************************************************************************/
    TRACE_INFO(( "subtree is heavy on left side" ));
    TRACE_INFO(( "left subtree" ));
    TRACE_INFO(( "  r_height = %d", (*subtree)->left->right_height));
    TRACE_INFO(( "  l_height = %d", (*subtree)->left->left_height));
    if ((*subtree)->left->right_height > (*subtree)->left->left_height)
    {
      /***********************************************************************/
      /* left subtree is heavier on right side, so must perform left         */
      /* rotation on this subtree to make it heavier on the left side        */
      /***********************************************************************/
      TRACE_INFO(( "left subtree is heavier on right side ..." ));
      TRACE_INFO(( "... so rotate it left" ));
      avl3_rotate_left(&(*subtree)->left);
      TRACE_INFO(( "left subtree" ));
      TRACE_INFO(( "  r_height = %d",
                                                  (*subtree)->left->right_height));
      TRACE_INFO(( "  l_height = %d",
                                                  (*subtree)->left->left_height));
    }

    /*************************************************************************/
    /* now rotate the subtree right                                          */
    /*************************************************************************/
    TRACE_INFO(( "rotate subtree right" ));
    avl3_rotate_right(subtree);
  }

  TRACE_INFO(( "balanced subtree" ));
  TRACE_INFO(( "  r_height = %d", (*subtree)->right_height ));
  TRACE_INFO(( "  l_height = %d", (*subtree)->left_height ));

  TRACE_EXIT();

  return;

} /* avl3_rebalance */


/**
 *  @brief Rotate a subtree of the AVL3 tree right
 *
 *  @param **subtree a pointer to the subtree to rotate
 *  @return None
 */
VOID avl3_rotate_right(HM_AVL3_NODE **subtree)
{
  /***************************************************************************/
  /* rotate subtree of AVL3 tree right                                       */
  /***************************************************************************/
  HM_AVL3_NODE *left_son;

  TRACE_ENTRY();

  left_son = (*subtree)->left;

  (*subtree)->left = left_son->right;
  if ((*subtree)->left != NULL)
  {
    TRACE_INFO(( "left son exists"));
    TRACE_ASSERT((*subtree)->left != *subtree);
    (*subtree)->left->parent = (*subtree);
  }
  (*subtree)->left_height = left_son->right_height;

  TRACE_ASSERT(left_son != (*subtree)->parent);
  left_son->parent = (*subtree)->parent;

  left_son->right = *subtree;
  TRACE_ASSERT(left_son->right != left_son);
  left_son->right->parent = left_son;
  left_son->right_height = (uint16_t)
                     (1 + MAX((*subtree)->right_height, (*subtree)->left_height));

  *subtree = left_son;

  TRACE_EXIT();

  return;

} /* avl3_rotate_right */

/**
 *  @brief Rotate a subtree of the AVL3 tree left
 *
 *  @param **subtree a pointer to the subtree to rotate
 *  @return None
 */
VOID avl3_rotate_left(HM_AVL3_NODE **subtree)
{
  /***************************************************************************/
  /* rotate a subtree of the AVL tree left                                   */
  /***************************************************************************/
  HM_AVL3_NODE *right_son;

  TRACE_ENTRY();

  right_son = (*subtree)->right;

  (*subtree)->right = right_son->left;
  if ((*subtree)->right != NULL)
  {
    TRACE_INFO(( "right son exists"));
    TRACE_ASSERT((*subtree)->right != *subtree);
    (*subtree)->right->parent = (*subtree);
  }
  (*subtree)->right_height = right_son->left_height;

  TRACE_ASSERT(right_son !=(*subtree)->parent);
  right_son->parent = (*subtree)->parent;

  right_son->left = *subtree;
  TRACE_ASSERT(right_son->left != right_son);
  right_son->left->parent = right_son;
  right_son->left_height = (uint16_t)
                     (1 + MAX((*subtree)->right_height, (*subtree)->left_height));

  *subtree = right_son;

  TRACE_ASSERT(*subtree != NULL);

  TRACE_EXIT();

  return;

} /* avl3_rotate_left */


/**
 *  @brief Swap node with right-most descendent of subtree
 *
 *  @param *tree a pointer to the tree
 *  @param *subtree  a pointer to the subtree
 *  @param *node a pointer to the node to swap
 *
 *  @return None
 */
VOID avl3_swap_right_most(HM_AVL3_TREE *tree,
                                  HM_AVL3_NODE *subtree,
                                  HM_AVL3_NODE *node)
{
  /***************************************************************************/
  /* swap node with right-most descendent of specified subtree               */
  /***************************************************************************/
  HM_AVL3_NODE *swap_node;
  HM_AVL3_NODE *swap_parent;
  HM_AVL3_NODE *swap_left;

  TRACE_ENTRY();

  TRACE_ASSERT(node->right != NULL);
  TRACE_ASSERT(node->left != NULL);

  /***************************************************************************/
  /* find right-most descendent of subtree                                   */
  /***************************************************************************/
  swap_node = subtree;
  while (swap_node->right != NULL)
  {
    /*************************************************************************/
    /* FLOW TRACING NOT REQUIRED    Reason: Tight loop.                      */
    /*************************************************************************/
    swap_node = swap_node->right;
  }

  TRACE_ASSERT(swap_node->right_height == 0);
  TRACE_ASSERT(swap_node->left_height <= 1);

  /***************************************************************************/
  /* save parent and left-son of right-most descendent                       */
  /***************************************************************************/
  swap_parent = swap_node->parent;
  swap_left = swap_node->left;

  /***************************************************************************/
  /* move swap node to its new position                                      */
  /***************************************************************************/
  TRACE_ASSERT(swap_node != node->parent);
  TRACE_ASSERT(swap_node != node->left);
  TRACE_ASSERT(swap_node != node->right);
  swap_node->parent = node->parent;
  swap_node->right = node->right;
  swap_node->left = node->left;
  swap_node->right_height = node->right_height;
  swap_node->left_height = node->left_height;
  swap_node->right->parent = swap_node;
  swap_node->left->parent = swap_node;
  if (node->parent == NULL)
  {
    /*************************************************************************/
    /* node is at root of tree                                               */
    /*************************************************************************/
    TRACE_INFO(( "Node is root"));
    tree->root = swap_node;
  }
  else if (node->parent->right == node)
  {
    /*************************************************************************/
    /* node is right-son of parent                                           */
    /*************************************************************************/
    TRACE_INFO(( "Node is right-son"));
    swap_node->parent->right = swap_node;
  }
  else
  {
    /*************************************************************************/
    /* node is left-son of parent                                            */
    /*************************************************************************/
    TRACE_INFO(( "Node is left-son"));
    swap_node->parent->left = swap_node;
  }

  /***************************************************************************/
  /* move node to its new position                                           */
  /***************************************************************************/
  TRACE_ASSERT(node != swap_parent);
  TRACE_ASSERT(node != swap_left);
  node->parent = swap_parent;
  node->right = NULL;
  node->left = swap_left;
  if (node->left != NULL)
  {
    TRACE_INFO(( "node has left-son"));
    node->left->parent = node;
    node->left_height = 1;
  }
  else
  {
    TRACE_INFO(( "node has no left-son"));
    node->left_height = 0;
  }
  node->right_height = 0;
  node->parent->right = node;

  TRACE_EXIT();

  return;

} /* avl3_swap_right_most */

/**
 *  @brief Swap node with left-most descendent of subtree
 *
 *  @param *tree A pointer to the tree
 *  @param *subtree a pointer to the subtree
 *  @param *node a pointer to the node to swap
 *
 *  @return None
 */
VOID avl3_swap_left_most(HM_AVL3_TREE *tree,
                                 HM_AVL3_NODE *subtree,
                                 HM_AVL3_NODE *node)
{
  /***************************************************************************/
  /* swap node with left-most descendent of specified subtree                */
  /***************************************************************************/
  HM_AVL3_NODE *swap_node;
  HM_AVL3_NODE *swap_parent;
  HM_AVL3_NODE *swap_right;

  TRACE_ENTRY();

  TRACE_ASSERT(node->right != NULL);
  TRACE_ASSERT(node->left != NULL);

  /***************************************************************************/
  /* find left-most descendent of subtree                                    */
  /***************************************************************************/
  swap_node = subtree;
  while (swap_node->left != NULL)
  {
    /*************************************************************************/
    /* FLOW TRACING NOT REQUIRED    Reason: Tight loop.                      */
    /*************************************************************************/
    swap_node = swap_node->left;
  }

  TRACE_ASSERT(swap_node->left_height == 0);
  TRACE_ASSERT(swap_node->right_height <= 1);

  /***************************************************************************/
  /* save parent and right-son of left-most descendent                       */
  /***************************************************************************/
  swap_parent = swap_node->parent;
  swap_right = swap_node->right;

  /***************************************************************************/
  /* move swap node to its new position                                      */
  /***************************************************************************/
  TRACE_ASSERT(swap_node != node->parent);
  TRACE_ASSERT(swap_node != node->left);
  TRACE_ASSERT(swap_node != node->right);
  swap_node->parent = node->parent;
  swap_node->right = node->right;
  swap_node->left = node->left;
  swap_node->right_height = node->right_height;
  swap_node->left_height = node->left_height;
  swap_node->right->parent = swap_node;
  swap_node->left->parent = swap_node;
  if (node->parent == NULL)
  {
    /*************************************************************************/
    /* node is at root of tree                                               */
    /*************************************************************************/
    TRACE_INFO(( "Node is root"));
    tree->root = swap_node;
  }
  else if (node->parent->right == node)
  {
    /*************************************************************************/
    /* node is right-son of parent                                           */
    /*************************************************************************/
    TRACE_INFO(( "Node is right-son"));
    swap_node->parent->right = swap_node;
  }
  else
  {
    /*************************************************************************/
    /* node is left-son of parent                                            */
    /*************************************************************************/
    TRACE_INFO(( "Node is left-son"));
    swap_node->parent->left = swap_node;
  }

  /***************************************************************************/
  /* move node to its new position                                           */
  /***************************************************************************/
  //NBB_ASSERT_PTR_NE(node, swap_parent);
  //NBB_ASSERT_PTR_NE(node, swap_right);
  node->parent = swap_parent;
  node->right = swap_right;
  node->left = NULL;
  if (node->right != NULL)
  {
    TRACE_INFO(( "node has right-son"));
    node->right->parent = node;
    node->right_height = 1;
  }
  else
  {
    TRACE_INFO(( "node has no right-son"));
    node->right_height = 0;
  }
  node->left_height = 0;
  node->parent->left = node;

  TRACE_EXIT();

  return;

} /* avl3_swap_left_most */


/**
 *  @brief Find the successor node to the supplied key in the AVL3 tree
 *
 *  @param *tree  a pointer to the AVL3 tree
 *  @param *key a pointer to the key
 *  @param not_equal TRUE return a node strictly > key
 *           FALSE return a node >= key
 *  @param *tree_info  a pointer to the AVL3 tree info
 *
 *  @return A pointer to the node. @c NULL if no successor node to the supplied key is found
 */
VOID *avl3_find_or_find_next(HM_AVL3_TREE *tree,
                                     VOID *key,
                                     uint32_t not_equal,
                                     const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  HM_AVL3_NODE *node;
  VOID *found_node = NULL;
  int32_t result;

  TRACE_ENTRY();

  TRACE_INFO(( "Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  node = tree->root;

  if (node != NULL)
  {
    /*************************************************************************/
    /* There is something in the tree                                        */
    /*************************************************************************/
    TRACE_INFO(( "Tree not empty"));
    for (;;)
    {
      /***********************************************************************/
      /* FLOW TRACING NOT REQUIRED           Reason: Performance             */
      /*                                                                     */
      /* compare key of current node with supplied key                       */
      /***********************************************************************/
      result = tree_info->compare(key,
                                  (VOID *)((BYTE *)node -
                                               tree_info->node_offset +
                                               tree_info->key_offset)
                                  );

      if (result > 0)
      {
        /*********************************************************************/
        /* specified key is greater than key of this node, so look in right  */
        /* subtree                                                           */
        /*********************************************************************/
        TRACE_INFO(( "move down right subtree" ));
        if (node->right == NULL)
        {
          /*******************************************************************/
          /* We've found the previous node - so we now need to find the      */
          /* successor to this one.                                          */
          /*******************************************************************/
          TRACE_INFO(( "Found previous node"));
          found_node = avl3_next(node, tree_info );
          break;
        }
        node = node->right;
      }
      else if (result < 0)
      {
        /*********************************************************************/
        /* specified key is less than key of this node, so look in left      */
        /* subtree                                                           */
        /*********************************************************************/
        TRACE_INFO(( "move down left subtree" ));

        if (node->left == NULL)
        {
           /******************************************************************/
           /* We've found the next node so store and drop out                */
           /******************************************************************/
           TRACE_INFO(( "Found next node"));
           found_node = (VOID *)
                                   ((BYTE *)node - tree_info->node_offset);
           break;
        }
        node = node->left;
      }
      else
      {
        /*********************************************************************/
        /* found the requested node                                          */
        /*********************************************************************/
        TRACE_INFO(( "found node for suplied key" ));

        if (not_equal)
        {
          /*******************************************************************/
          /* need to find the successor node to this node                    */
          /*******************************************************************/
          TRACE_INFO(( "next node required"));
          found_node = avl3_next(node, tree_info );
        }
        else
        {
          TRACE_INFO(( "return the exact match"));
          found_node = (VOID *)((BYTE *)node - tree_info->node_offset);
        }
        break;
      }
    }
  }

  TRACE_EXIT();

  return(found_node);

} /* avl3_find_or_find_next */

/**
 *  @brief To verify that an AVL3 tree is correctly sorted.
 *
 *  @param *tree a pointer to the AVL3 tree
 *  @param *tree_info  a pointer to the AVL3 tree info
 *  @return None
 */
VOID avl3_verify_tree(HM_AVL3_TREE *tree,
                              const HM_AVL3_TREE_INFO *tree_info)
{
  /***************************************************************************/
  /* Local Variables                                                         */
  /***************************************************************************/
  HM_AVL3_NODE *node = NULL;
  HM_AVL3_NODE *tmpnode = NULL;
  int32_t result;

  TRACE_ENTRY();

  TRACE_INFO(( "Compare fn %p; key offset %hu; node offset %hu",
                  tree_info->compare,
                  tree_info->key_offset,
                  tree_info->node_offset));

  /***************************************************************************/
  /* Assert that the root node has no parent.                                */
  /***************************************************************************/
  TRACE_ASSERT(((tree->root == NULL) || (tree->root->parent == NULL)));

  /***************************************************************************/
  /* The first node is the leftmost node in the tree - following the left    */
  /* pointers from the root node ends at the first node, and following the   */
  /* parent pointers from the first node ends at the root node.              */
  /*                                                                         */
  /* Check that this chain from the first node to the root node is correct   */
  /* by following the parent pointers from the first node to the root node   */
  /* and asserting the left pointers at each step.                           */
  /*                                                                         */
  /* Within the loop node is the parent of tmpnode and tmpnode should be the */
  /* left child of node.  At the end of the loop node is NULL and tmpnode    */
  /* should be the root node.                                                */
  /***************************************************************************/
  tmpnode = NULL;
  for (node = tree->first; node != NULL; node = node->parent)
  {
    /*************************************************************************/
    /* FLOW TRACING NOT REQUIRED Reason: Performance                         */
    /*************************************************************************/
    TRACE_ASSERT(node->left == tmpnode);
    TRACE_ASSERT(node != node->parent);
    TRACE_ASSERT(node != node->left);
    tmpnode = node;
  }
  TRACE_ASSERT(tree->root == tmpnode);

  /***************************************************************************/
  /* Now we've ascertained that the first and root are correct, check the    */
  /* relative ordering of everything else in the tree.                       */
  /***************************************************************************/
  node = tree->first;

  while (node != NULL)
  {
    /*************************************************************************/
    /* FLOW TRACING NOT REQUIRED Reason: Performance                         */
    /*************************************************************************/
    tmpnode = (HM_AVL3_NODE *)HM_AVL3_NEXT(*node, *tree_info);

    /*************************************************************************/
    /* Get out if we have reach the end of the line.                         */
    /*************************************************************************/
    if (tmpnode == NULL)
    {
      /***********************************************************************/
      /* FLOW TRACING NOT REQUIRED Reason: Performance                       */
      /***********************************************************************/
      break;
    }

    /*************************************************************************/
    /* Check that node and its next node are ordered correctly.  Check the   */
    /* ordering in both directions to ensure there are no infinite loops.    */
    /*************************************************************************/
    tmpnode = (HM_AVL3_NODE *)(((BYTE *)tmpnode + tree_info->node_offset));
    result = tree_info->compare((VOID *)((BYTE *)node -
                                             tree_info->node_offset +
                                             tree_info->key_offset),
                                (VOID *)((BYTE *)tmpnode -
                                             tree_info->node_offset +
                                             tree_info->key_offset)
                                );
    //NBB_ASSERT_NUM_LT(result, 0);

    result = tree_info->compare((VOID *)((BYTE *)tmpnode -
                                             tree_info->node_offset +
                                             tree_info->key_offset),
                                (VOID *)((BYTE *)node -
                                             tree_info->node_offset +
                                             tree_info->key_offset)
                                );
    TRACE_ASSERT(result > 0);

    /*************************************************************************/
    /* Also check the parentage.                                             */
    /*************************************************************************/
    TRACE_ASSERT(((node->parent != NULL) || ( node == tree->root)));
    node = tmpnode;
  }

  /***************************************************************************/
  /* Finally, now we have completed the walk of the (potentially empty)      */
  /* tree, check that the last pointer is set correctly.                     */
  /***************************************************************************/
  TRACE_ASSERT(node == tree->last);

  /***************************************************************************/
  /* There is no need to walk through last's parentage to check them since   */
  /* we have already checked the correct ordering of all members of the tree */
  /* while we were walking.  This step is necessary for the first since it   */
  /* it the foundation upon which we base our tree walk to check all the     */
  /* relative orderings.                                                     */
  /***************************************************************************/

  TRACE_EXIT();

  return;

} /* avl3_verify_tree */

/**
 *  @brief Allocates and sets up a generic AVL3_NODE
 *
 *  @param *key Pointer to the key
 *  @param *parent Parent container of this node
 *
 *  @return a #HM_AVL3_GEN_NODE if successful, @c NULL otherwise
 */
HM_AVL3_GEN_NODE * hm_avl3_gen_init(void *key, void *parent)
{
  /***************************************************************************/
  /* Variable Declarations                           */
  /***************************************************************************/
  HM_AVL3_GEN_NODE *node = NULL;
  /***************************************************************************/
  /* Sanity Checks                               */
  /***************************************************************************/
  TRACE_ENTRY();

  TRACE_ASSERT(key != NULL);
  TRACE_ASSERT(parent != NULL);
  /***************************************************************************/
  /* Main Routine                                 */
  /***************************************************************************/
  node = (HM_AVL3_GEN_NODE *)malloc(sizeof(HM_AVL3_GEN_NODE));
  if(node == NULL)
  {
    TRACE_ERROR(("Error allocating memory for node."));
    goto EXIT_LABEL;
  }
  /***************************************************************************/
  /* Have memory, fill values                           */
  /***************************************************************************/
  HM_AVL3_INIT_NODE(node->tree_node, node);
  node->key = key;
  node->parent = parent;

EXIT_LABEL:
  /***************************************************************************/
  /* Exit Level Checks                             */
  /***************************************************************************/
  TRACE_EXIT();
  return node;
}/* hm_avl3_gen_init */
