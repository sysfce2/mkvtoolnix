/*
   mkvtoolnix - A set of programs for manipulating Matroska files

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Cross platform compatibility definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef __MTX_COMMON_OS_WINDOWS_H
#define __MTX_COMMON_OS_WINDOWS_H

#include "common/os.h"

#include <string>

#ifdef SYS_WINDOWS

int MTX_DLL_API fs_entry_exists(const char *path);
void MTX_DLL_API create_directory(const char *path);
int64_t MTX_DLL_API get_current_time_millis();

bool MTX_DLL_API get_registry_key_value(const std::string &key, const std::string &value_name, std::string &value);
std::string MTX_DLL_API get_installation_path();

void MTX_DLL_API set_environment_variable(const std::string &key, const std::string &value);
std::string MTX_DLL_API get_environment_variable(const std::string &key);

#endif  // SYS_WINDOWS

#endif  // __MTX_COMMON_OS_WINDOWS_H
