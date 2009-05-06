/*
   mkvmerge GUI -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   header editor: abstract base class for all pages

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/os.h"

#include <wx/textctrl.h>

#include "mmg/header_editor_frame.h"
#include "mmg/he_page_base.h"

he_page_base_c::he_page_base_c(header_editor_frame_c *parent)
  : wxPanel(parent->get_page_panel(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
  , m_parent(parent)
  , m_page_id()
  , m_l1_element(NULL)
{
  Hide();
}

he_page_base_c::~he_page_base_c() {
}

bool
he_page_base_c::has_been_modified() {
  if (has_this_been_modified())
    return true;

  std::vector<he_page_base_c *>::iterator it = m_children.begin();

  while (it != m_children.end()) {
    if ((*it)->has_been_modified())
      return true;
    ++it;
  }

  return false;
}

void
he_page_base_c::do_modifications() {
  modify_this();

  std::vector<he_page_base_c *>::iterator it = m_children.begin();

  while (it != m_children.end()) {
    (*it)->do_modifications();
    ++it;
  }
}

wxTreeItemId
he_page_base_c::validate() {
  if (!validate_this())
    return m_page_id;

  std::vector<he_page_base_c *>::iterator it = m_children.begin();

  while (it != m_children.end()) {
    wxTreeItemId result = (*it)->validate();
    if (result.IsOk())
      return result;
    ++it;
  }

  return wxTreeItemId();
}
