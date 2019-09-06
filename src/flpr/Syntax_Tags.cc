#include "Syntax_Tags.hh"
#include <cassert>

/* Declare the static member variable */
std::vector<FLPR::Syntax_Tags::Ext_Record> FLPR::Syntax_Tags::extensions_;

std::string FLPR::Syntax_Tags::label(int const syntag) {
  const int ext_idx = get_ext_idx_(syntag);
  if (ext_idx == -2)
    return strings_[syntag];
  if (ext_idx == -1) {
    std::string tmp =
        "<client-extension+" + std::to_string(syntag - CLIENT_EXTENSION) + '>';
    return tmp;
  }
  return extensions_[ext_idx].label;
}

int FLPR::Syntax_Tags::type(int const syntag) {
  const int ext_idx = get_ext_idx_(syntag);
  if (ext_idx == -2)
    return types_[syntag];
  if (ext_idx == -1)
    return 4;
  return extensions_[ext_idx].type;
}

bool FLPR::Syntax_Tags::register_ext(int const tag_idx, char const *const label,
                                     int const type) {
  assert(tag_idx >= CLIENT_EXTENSION);
  const size_t ext_idx = static_cast<size_t>(tag_idx - CLIENT_EXTENSION);
  if (extensions_.size() <= ext_idx) {
    extensions_.resize(ext_idx + 1);
  }
  assert(extensions_[ext_idx].empty());
  extensions_[ext_idx].label = std::string(label);
  extensions_[ext_idx].type = type;
  return true;
}
