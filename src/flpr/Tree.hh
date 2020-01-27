/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

/*!
   \file include/flpr/Tree.hh

   Classes related to the implementation of a generic tree structure.
*/

#ifndef FLPR_TREE_HH
#define FLPR_TREE_HH 1

#include <cassert>
#include <memory>
#include <ostream>
#include <type_traits>

#include "flpr/Safe_List.hh"

namespace FLPR {

template <class Tp, class Alloc> class Tree;

//! FLPR internal implementation details
namespace details_ {

template <class Tp, class Alloc> class TN_Cursor;
template <class Tp, class Alloc> class TN_Const_Cursor;

//! The implementation details for each node of FLPR::Tree
template <class Tp, class Alloc> class Tree_Node {
private:
  template <class, class> friend class TN_Cursor;
  template <class, class> friend class TN_Const_Cursor;
  template <class, class> friend class FLPR::Tree;

public:
  using allocator = typename std::allocator_traits<
      Alloc>::template rebind_alloc<Tree_Node<Tp, Alloc>>;
  using node_list = Safe_List<Tree_Node, allocator>;
  using iterator = typename node_list::iterator;
  using const_iterator = typename node_list::const_iterator;

  using value_type = Tp;
  using reference = value_type &;
  using const_reference = value_type const &;
  using pointer = value_type *;
  using const_pointer = value_type const *;

  Tree_Node() = default;
  Tree_Node(Tree_Node const &) = delete;
  Tree_Node(Tree_Node &&) = default;
  Tree_Node &operator=(Tree_Node const &) = delete;
  Tree_Node &operator=(Tree_Node &&) = default;

  // You must call link to finish constructing this class!
  template <class... Args>
  explicit constexpr Tree_Node(Args &&... args)
      : contents_{std::make_unique<Contents_>(std::forward<Args>(args)...)},
        linked_{false} {}

  explicit constexpr Tree_Node(value_type &&src)
      : contents_{std::make_unique<Contents_>(std::move(src))}, linked_{false} {
  }

  explicit constexpr Tree_Node(value_type const &src)
      : contents_{std::make_unique<Contents_>(src)}, linked_{false} {}

  constexpr void link(iterator self) noexcept {
    linked_ = true;
    self_itr_ = self;
    parent_ = self;
    fix_branches();
  }
  constexpr void link(iterator self, iterator parent) noexcept {
    linked_ = true;
    self_itr_ = self;
    parent_ = parent;
    fix_branches();
  }

  constexpr reference operator*() noexcept { return **contents_; }
  constexpr const_reference operator*() const noexcept { return **contents_; }
  constexpr pointer operator->() noexcept { return &(**contents_); }
  constexpr const_pointer operator->() const noexcept { return &(**contents_); }
  constexpr iterator trunk() {
    assert(linked_);
    assert(parent_ == parent_->self_itr_);
    return parent_;
  }
  constexpr const_iterator trunk() const {
    assert(linked_);
    assert(parent_ == parent_->self_itr_);
    return parent_;
  }
  constexpr iterator self() {
    assert(linked_);
    return self_itr_;
  }
  constexpr const_iterator self() const {
    assert(linked_);
    return self_itr_;
  }
  constexpr size_t num_branches() const noexcept {
    return contents_->num_branches();
  }
  //! Return the number of nodes in the subtree rooted here.
  size_t size() const noexcept {
    size_t count{1};
    if (is_fork()) {
      for (auto const &b : branches())
        count += b.size();
    }
    return count;
  }
  constexpr bool is_leaf() const noexcept {
    assert(contents_);
    return contents_->is_leaf();
  }
  constexpr bool is_fork() const noexcept { return !is_leaf(); }
  constexpr bool is_root() const noexcept {
    assert(linked_);
    return parent_ == self_itr_;
  }
  //! Return a const reference to the list of branches
  /*! Note that this will allocate the branch list if it doesn't already exist
      EVEN for a const node!  To save this unneeded memory on leaf nodes, avoid
      calls to branches if is_leaf().  This function is provided to enable
      things like "for(auto const &n: foo.branches()){}" */
  constexpr node_list const &branches() const {
    assert(contents_);
    return contents_->branches();
  }
  //! Non-const reference to the list of branches
  constexpr node_list &branches() {
    assert(contents_);
    return contents_->branches();
  }
  void check() const;
  iterator emplace(const_iterator pos, Tree_Node &&new_branch) {
    auto handle = branches().emplace(pos, std::move(new_branch));
    handle->link(handle, self());
    return handle;
  }
  iterator emplace_front(Tree_Node &&new_branch) {
    branches().emplace_front(std::move(new_branch));
    auto handle = branches().begin();
    handle->link(handle, self());
    return handle;
  }
  iterator emplace_back(Tree_Node &&new_branch) {
    branches().emplace_back(std::move(new_branch));
    auto handle = std::prev(branches().end());
    handle->link(handle, self());
    return handle;
  }
  constexpr void swap(Tree_Node &other) noexcept {
    contents_.swap(other.contents_);
    fix_branches();
    other.fix_branches();
  }

private:
  /*  NOTE NOTE NOTE FIX
     The use of std::unique_ptr in this implementation section is bad because
     the memory is NOT allocated from any supplied \c allocator (that's just how
     std::unique_ptr works).  This needs to switch to \c
     std::allocator_traits::construct.
  */

  //! Bundle up the value and branches to make for easy swapping
  class Contents_ {
  public:
    template <class... Args>
    explicit constexpr Contents_(Args &&... args)
        : value_(std::forward<Args>(args)...), branch_p_{} {}
    constexpr node_list &branches() {
      init_branches_();
      return *branch_p_;
    }
    constexpr size_t num_branches() const noexcept {
      return (branch_p_) ? branch_p_->size() : 0;
    }
    constexpr reference operator*() { return value_; }
    constexpr pointer operator->() { return &value_; }
    constexpr bool is_leaf() const noexcept {
      return (!branch_p_ || branch_p_->empty());
    }

  private:
    //! The client data
    value_type value_;

    //! Pointer to node_list of branches
    /*! We use a pointer here because there are (often) many leaf nodes, and we
      don't want to pay the overhead for unused branch lists in these nodes.  */
    std::unique_ptr<node_list> branch_p_;

    //! Create branch list, if needed
    constexpr void init_branches_() {
      if (!branch_p_) {
        branch_p_ = std::make_unique<node_list>();
      }
    }
  }; // Contents_

  //! Pointer to the contents
  /*! We organize things in this way to make it easy to swap the contents of two
      nodes without needing Tp to be swappable. It also cleans up the notion of
      disconnecting the contents from a tree, in which case it no longer has a
      self_itr_ or parent_. */
  std::unique_ptr<Contents_> contents_;

  /*! @name treenode_links Tree_Node Link Variables
    In addition to branches/children, each Tree_Node contains a link up to its
    parent (or self, if a root node), and a self-link.  The self-link is useful
    when you want to insert a new node around this one, or to detach or erase a
    given node: e.g. \c this->parent_->branches().erase(this->self_itr_) */
  //! @{
  //! True if the parent_ and self_itr_ links have been initialized
  bool linked_{false};
  //! Link to my parent in the tree
  iterator parent_;
  //! The iterator referencing *this in the parent_.contents_.branches() list
  iterator self_itr_;
  //! @}

  //! Fix the up links of my branches (use when this->self_itr_ changes)
  constexpr void fix_branches() noexcept {
    if (contents_ && is_fork()) {
      for (iterator i = branches().begin(); i != branches().end(); ++i) {
        i->parent_ = self_itr_;
      }
    }
  }
};

template <class Tp, class Alloc> void Tree_Node<Tp, Alloc>::check() const {
  assert(linked_);
  assert(contents_);
  if (!contents_->is_leaf()) {
    assert(!contents_->branches().empty());
    for (auto bi = contents_->branches().begin();
         bi != contents_->branches().end(); ++bi) {
      assert(bi == bi->self_itr_);
      assert(bi->parent_ == self_itr_);
      bi->check();
    }
  }
}

template <class Tp, class Alloc>
std::ostream &operator<<(std::ostream &os, Tree_Node<Tp, Alloc> const &tn) {
  os << *tn;
  if (tn.is_fork()) {
    os << " <";
    for (auto const &b : tn.branches()) {
      os << b << ' ';
    }
    os << ">";
  }
  return os;
}

//! A convenience mechanism for moving around a Tree
/*! A Tree_Node Cursor, or TN_Cursor, is intended to simplify the tasks of
    moving around a Tree and accessing node data.  The basic model is a
    multi-dimensional iterator.  Starting on some node, the TN_Cursor provides
    neighbor existence queries (has_up(), has_down(), has_prev(), has_next()),
    checked updates (up(), down(), prev(), next()), Tree_Node characteristics
    (is_root(), is_fork() (has children), is_leaf() (has no children)), and data
    access members.
 */
template <class Tp, class Alloc> class TN_Cursor {
public:
  using node_t = Tree_Node<Tp, Alloc>;
  using node_list = typename node_t::node_list;
  using iterator = typename node_list::iterator;
  using const_iterator = typename node_list::const_iterator;
  using value_type = typename node_t::value_type;
  using reference = typename node_t::reference;
  using pointer = typename node_t::pointer;
  using const_pointer = typename node_t::const_pointer;

  constexpr TN_Cursor() : iter_{}, assoc_{false} {}
  constexpr explicit TN_Cursor(iterator pos) : iter_{pos}, assoc_{true} {}

  //! Return true if this node is the root of the Tree
  constexpr bool is_root() const noexcept {
    assert(assoc_);
    return iter_->is_root();
  }
  //! Return true if this node has descendent branches
  constexpr bool is_fork() const noexcept {
    assert(assoc_);
    return iter_->is_fork();
  }
  //! Return the number of descendent branches
  constexpr size_t num_branches() const noexcept {
    assert(assoc_);
    return iter_->num_branches();
  }
  //! Return true if this node has no descendent branches
  constexpr bool is_leaf() const noexcept {
    assert(assoc_);
    return iter_->is_leaf();
  }
  //! Return true if this TN_Cursor is associated with a Tree_Node
  constexpr operator bool() const noexcept { return assoc_; }
  //! Unassociate from any Tree_Node
  constexpr void clear() noexcept { assoc_ = false; }

  //! Return true if there is an ascendant node (a level above this one)
  [[nodiscard]] constexpr bool has_up() const noexcept {
    assert(assoc_);
    return !iter_->is_root();
  }
  //! Move up \p count levels
  constexpr TN_Cursor &up(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_up());
      iter_ = iter_->trunk();
    }
    return *this;
  }
  //! Return true if there is a predecessor in the list of nodes at this level
  [[nodiscard]] constexpr bool has_prev() const noexcept {
    assert(assoc_);
    return !iter_->is_root() && (iter_->parent_->branches().begin() != iter_);
  }
  //! Move backwards \p count predecessors
  constexpr TN_Cursor &prev(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_prev());
      iter_ = std::prev(iter_);
    }
    return *this;
  }
  //! Return true is there is a successor in the list of nodes at this level
  [[nodiscard]] constexpr bool has_next() const noexcept {
    assert(assoc_);
    return !iter_->is_root() &&
           (iter_->parent_->branches().end() != std::next(iter_));
  }
  //! Move forward \p count successors
  constexpr TN_Cursor &next(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_next());
      iter_ = std::next(iter_);
    }
    return *this;
  }
  //! Try to move forward \p count successors
  constexpr bool try_next(int const count = 1) noexcept {
    int i{0};
    for (i = 0; i < count && has_next(); ++i) {
      iter_ = std::next(iter_);
    }
    return i == count;
  }
  //! Return true if there is a descendent node (a level below this one)
  [[nodiscard]] constexpr bool has_down() const noexcept {
    assert(assoc_);
    return iter_->is_fork();
  }
  //! Move down \p count descendent levels
  constexpr TN_Cursor &down(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_down());
      iter_ = iter_->branches().begin();
    }
    return *this;
  }
  //! Try to move down \p count descendent levels
  constexpr bool try_down(int const count = 1) noexcept {
    int i{0};
    for (i = 0; i < count && has_down(); ++i) {
      iter_ = iter_->branches().begin();
    }
    return i == count;
  }
  //! Return a reference to the Tree_Node that the cursor is currently on
  [[nodiscard]] constexpr typename iterator::reference node() noexcept {
    assert(assoc_);
    return *iter_;
  }
  //! Return a const reference to the Tree_Node that the cursor is currently on
  [[nodiscard]] constexpr typename const_iterator::reference node() const
      noexcept {
    assert(assoc_);
    return *iter_;
  }
  //! Return a reference to the current Tree_Node DATA
  [[nodiscard]] constexpr reference operator*() noexcept { return *node(); }
  //! Return a pointer to the current Tree_Node DATA
  [[nodiscard]] constexpr pointer operator->() noexcept { return &(*node()); }
  //! Return a const pointer to the current Tree_Node DATA
  [[nodiscard]] constexpr const_pointer operator->() const noexcept {
    return &(*node());
  }
  //! Return the current Tree_Node::node_list::iterator
  [[nodiscard]] constexpr iterator self() noexcept {
    assert(assoc_);
    return iter_;
  }
  //! Return the current Tree_Node::node_list::const_iterator
  [[nodiscard]] constexpr const_iterator self() const noexcept {
    assert(assoc_);
    return iter_;
  }

private:
  iterator iter_;
  bool assoc_;
};

//! A const version of TN_Cursor
template <class Tp, class Alloc> class TN_Const_Cursor {
public:
  using node_t = Tree_Node<Tp, Alloc>;
  using node_list = typename node_t::node_list;
  using iterator = typename node_list::const_iterator;
  using value_type = typename node_t::value_type;
  using reference = typename node_t::const_reference;
  using pointer = typename node_t::const_pointer;

  constexpr TN_Const_Cursor() : iter_{}, assoc_{false} {}
  constexpr explicit TN_Const_Cursor(iterator pos) : iter_{pos}, assoc_{true} {}
  constexpr TN_Const_Cursor(TN_Cursor<Tp, Alloc> const &c)
      : iter_{c.self()}, assoc_{true} {}
  //! Return true if this node is the root of the Tree
  constexpr bool is_root() const noexcept {
    assert(assoc_);
    return iter_->is_root();
  }
  //! Return true if this node has descendent branches
  constexpr bool is_fork() const noexcept {
    assert(assoc_);
    return iter_->is_fork();
  }
  //! Return the number of descendent branches
  constexpr size_t num_branches() const noexcept {
    assert(assoc_);
    return iter_->num_branches();
  }
  //! Return true if this node has no descendent branches
  constexpr bool is_leaf() const noexcept {
    assert(assoc_);
    return iter_->is_leaf();
  }
  //! Return true if this TN_Const_Cursor is associated with a Tree_Node
  constexpr operator bool() const noexcept { return assoc_; }
  //! Unassociate from any Tree_Node
  constexpr void clear() noexcept { assoc_ = false; }

  //! Return true if there is an ascendant node (a level above this one)
  [[nodiscard]] constexpr bool has_up() const noexcept {
    assert(assoc_);
    return !iter_->is_root();
  }
  //! Move up \p count levels
  constexpr TN_Const_Cursor &up(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_up());
      iter_ = iter_->trunk();
    }
    return *this;
  }
  //! Return true if there is a predecessor in the list of nodes at this level
  [[nodiscard]] constexpr bool has_prev() const noexcept {
    assert(assoc_);
    return !iter_->is_root() && (iter_->parent_->branches().begin() != iter_);
  }
  //! Move backwards \p count predecessors
  constexpr TN_Const_Cursor &prev(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_prev());
      iter_ = std::prev(iter_);
    }
    return *this;
  }
  //! Return true is there is a successor in the list of nodes at this level
  [[nodiscard]] constexpr bool has_next() const noexcept {
    assert(assoc_);
    return !iter_->is_root() &&
           (iter_->parent_->branches().end() != std::next(iter_));
  }
  //! Move forward \p count successors
  constexpr TN_Const_Cursor &next(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_next());
      iter_ = std::next(iter_);
    }
    return *this;
  }
  //! Try to move forward \p count successors
  constexpr bool try_next(int const count = 1) noexcept {
    int i{0};
    for (i = 0; i < count && has_next(); ++i) {
      iter_ = std::next(iter_);
    }
    return i == count;
  }
  //! Return true if there is a descendent node (a level below this one)
  [[nodiscard]] constexpr bool has_down() const noexcept {
    assert(assoc_);
    return iter_->is_fork();
  }
  //! Move down \p count descendent levels
  constexpr TN_Const_Cursor &down(int const count = 1) noexcept {
    for (int i = 0; i < count; ++i) {
      assert(has_down());
      iter_ = iter_->branches().begin();
    }
    return *this;
  }
  //! Try to move down \p count descendent levels
  constexpr bool try_down(int const count = 1) noexcept {
    int i{0};
    for (i = 0; i < count && has_down(); ++i) {
      iter_ = iter_->branches().begin();
    }
    return i == count;
  }
  //! Return a reference to the Tree_Node that the cursor is currently on
  [[nodiscard]] constexpr typename iterator::reference node() const noexcept {
    assert(assoc_);
    return *iter_;
  }
  //! Return a reference to the current Tree_Node DATA
  [[nodiscard]] constexpr reference operator*() const noexcept {
    return *node();
  }
  //! Return a pointer to the current Tree_Node DATA
  [[nodiscard]] constexpr pointer operator->() const noexcept {
    return &(*node());
  }
  //! Return the current Tree_Node::node_list::iterator
  [[nodiscard]] constexpr iterator self() const noexcept {
    assert(assoc_);
    return iter_;
  }

private:
  iterator iter_;
  bool assoc_;
};

} // namespace details_

//! A generic tree data structure
/*! Each node can have an arbitrary number of branches (children), and user data
 *  of type \c Tp is stored at each node.
 */
template <class Tp, class Alloc = std::allocator<Tp>> class Tree {
public:
  using node = details_::Tree_Node<Tp, Alloc>;
  using node_list = typename node::node_list;
  using iterator = typename node_list::iterator;
  using const_iterator = typename node_list::const_iterator;
  using reference = node &;
  using const_reference = node const &;
  using pointer = node *;
  using const_pointer = node const *;
  using cursor_t = details_::TN_Cursor<Tp, Alloc>;
  using const_cursor_t = details_::TN_Const_Cursor<Tp, Alloc>;
  using value = Tp;

  static_assert(std::is_default_constructible_v<value>);

  Tree() = default;
  Tree(Tree const &) = delete;
  Tree(Tree &&) = default;
  Tree &operator=(Tree const &) = delete;
  Tree &operator=(Tree &&) = default;

  template <class... Args>
  constexpr explicit Tree(Args &&... args)
      : root_list_p_{std::make_unique<node_list>()} {
    root_list_p_->emplace_front(std::forward<Args>(args)...);
    root_list_p_->front().link(root_list_p_->begin());
  }

  constexpr explicit Tree(value const &src)
      : root_list_p_{std::make_unique<node_list>()} {
    root_list_p_->push_front(node(src));
    root_list_p_->front().link(root_list_p_->begin());
  }

  constexpr explicit Tree(value &&src)
      : root_list_p_{std::make_unique<node_list>()} {
    root_list_p_->emplace_front(std::move(src));
    root_list_p_->front().link(root_list_p_->begin());
  }

  //! If val is \c true, default construct a root node
  /*! Note that this differs from Tree(), which does NOT allocate a root_list.
      Here, we have a valid root_node which can be altered later. */
  constexpr explicit Tree(bool val)
      : root_list_p_{std::make_unique<node_list>()} {
    if (val) {
      root_list_p_->emplace_front(value{});
      root_list_p_->front().link(root_list_p_->begin());
    }
  }
  constexpr operator bool() const noexcept {
    return static_cast<bool>(root_list_p_);
  }
  constexpr bool tree_initialized() const noexcept {
    return static_cast<bool>(root_list_p_);
  }
  constexpr iterator root() noexcept { return iterator(root_node().self_itr_); }
  constexpr const_iterator root() const noexcept {
    assert(tree_initialized());
    return const_iterator(root_node().self_itr_);
  }
  constexpr const_iterator croot() const noexcept {
    assert(tree_initialized());
    return const_iterator(root_node().self_itr_);
  }

  //! Return true if the tree has no nodes
  constexpr bool empty() const noexcept {
    return !tree_initialized() || root_list_p_->empty();
  }
  //! Return the number of nodes in the tree
  size_t size() const noexcept {
    if (empty())
      return 0;
    return root_node().size();
  }
  constexpr void clear() noexcept { root_list_p_.reset(); }
  constexpr reference operator*() { return root_node(); }
  constexpr const_reference operator*() const { return root_node(); }
  constexpr pointer operator->() { return &(root_node()); }
  constexpr const_pointer operator->() const { return &(root_node()); }

  constexpr cursor_t cursor() noexcept {
    return cursor_t(root_node().self_itr_);
  }
  constexpr const_cursor_t cursor() const noexcept {
    return const_cursor_t(root_node().self_itr_);
  }
  constexpr const_cursor_t ccursor() const noexcept {
    return const_cursor_t(root_node().self_itr_);
  }

  //! graft a subtree on as a branch of this root
  /*! Reference version: this steals the root_node from donor and clears
      donor */
  iterator graft(iterator it, Tree &donor) {
    iterator retval =
        root_node().branches().emplace(it, std::move(donor.root_node()));
    retval->link(retval, root());
    donor.clear();
    return retval;
  }
  constexpr iterator graft_front(Tree &donor) {
    return graft(root_node().branches().begin(), donor);
  }
  constexpr iterator graft_back(Tree &donor) {
    return graft(root_node().branches().end(), donor);
  }

  //! Move version: this moves the root_node
  constexpr iterator graft(const_iterator pos, Tree &&donor) {
    iterator retval = root_node().emplace(pos, std::move(donor.root_node()));
    retval->link(retval, root());
    return retval;
  }
  constexpr iterator graft_front(Tree &&donor) {
    return graft(root_node().branches().begin(), std::move(donor));
  }
  constexpr iterator graft_back(Tree &&donor) {
    return graft(root_node().branches().end(), std::move(donor));
  }

  void swap(Tree &other) {
    /* exchanging the root_list_p_ pointers doesn't invalidate the self_itr_ or
       parent_ members of the nodes because this swap doesn't change the
       root_lists themselves. */
    root_list_p_.swap(other.root_list_p_);
  }

  void check() const;

private:
  //! Pointer to the \c node_list which contains only the root node
  /*! A couple of notes:
   *  - We're using a Tree_Node::node_list here to hold the root because we want
   *    every Tree_Node to have a Tree_Node::node_list::iterator (including the
   *    root node).
   *  - We're using a std::unique_ptr here for the root node list because we
   *    want very lightweight copies of empty trees. Could use std::optional.
   */
  std::unique_ptr<node_list> root_list_p_;

  //! Return a reference to the single root node in the root_list
  constexpr node &root_node() {
    assert(tree_initialized());
    assert(!root_list_p_->empty());
    return root_list_p_->front();
  }

  //! Return a const reference to the single root node in the root_list
  constexpr node const &root_node() const {
    assert(tree_initialized());
    assert(!root_list_p_->empty());
    return root_list_p_->front();
  }
};

template <class Tp, class Alloc> void Tree<Tp, Alloc>::check() const {
  if (root_list_p_) {
    assert(root_list_p_->size() == 1);
    root_node().check();
  }
}

template <class Tp, class Alloc>
std::ostream &operator<<(std::ostream &os, Tree<Tp, Alloc> const &t) {
  if (!t)
    return os << "<empty>";
  return os << *t;
}

} // namespace FLPR

#endif
