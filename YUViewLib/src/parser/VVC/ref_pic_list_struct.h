/*  This file is part of YUView - The YUV player with advanced analytics toolset
 *   <https://github.com/IENT/YUView>
 *   Copyright (C) 2015  Institut f�r Nachrichtentechnik, RWTH Aachen University, GERMANY
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   In addition, as a special exception, the copyright holders give
 *   permission to link the code of portions of this program with the
 *   OpenSSL library under certain conditions as described in each
 *   individual source file, and distribute linked combinations including
 *   the two.
 *
 *   You must obey the GNU General Public License in all respects for all
 *   of the code used other than OpenSSL. If you modify file(s) with this
 *   exception, you may extend this exception to your version of the
 *   file(s), but you are not obligated to do so. If you do not wish to do
 *   so, delete this exception statement from your version. If you delete
 *   this exception statement from all source files in the program, then
 *   also delete it here.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "parser/common/ReaderHelperNew.h"

namespace parser::vvc
{

class seq_parameter_set_rbsp;

class ref_pic_list_struct
{
public:
  ref_pic_list_struct()  = default;
  ~ref_pic_list_struct() = default;
  void parse(reader::ReaderHelperNew &reader,
             unsigned                 listIdx,
             unsigned                 rplsIdx,
             seq_parameter_set_rbsp * sps);

  umap_2d<unsigned> num_ref_entries{};
  umap_2d<bool>     ltrp_in_header_flag{};
  umap_3d<bool>     inter_layer_ref_pic_flag{};
  umap_3d<bool>     st_ref_pic_flag{};
  umap_3d<unsigned> abs_delta_poc_st{};
  umap_3d<bool>     strp_entry_sign_flag{};
  umap_3d<unsigned> rpls_poc_lsb_lt{};
  umap_3d<unsigned> ilrp_idx{};

  umap_3d<bool> AbsDeltaPocSt;
  umap_3d<int>  DeltaPocValSt;
};

} // namespace parser::vvc
