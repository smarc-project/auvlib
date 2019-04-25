/* Copyright 2018 Nils Bore (nbore@kth.se)
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NAVI_DATA_H
#define NAVI_DATA_H

#include <eigen3/Eigen/Dense>
#include <data_tools/std_data.h>

namespace navi_data {

using TransT = std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> >;
using RotsT = std::vector<Eigen::Matrix3d, Eigen::aligned_allocator<Eigen::Matrix3d> >;
using AngsT = std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> >;
using ObsT = std::vector<Eigen::MatrixXd, Eigen::aligned_allocator<Eigen::MatrixXd> >;
using MatchesT = std::vector<std::pair<int, int> >; // tells us which maps overlap
using BBsT = std::vector<Eigen::Matrix2d, Eigen::aligned_allocator<Eigen::Matrix2d> >;

void match_timestamps(std_data::mbes_ping::PingsT& pings, std_data::nav_entry::EntriesT& entries);

void divide_tracks(std_data::mbes_ping::PingsT& pings);
void divide_tracks_equal(std_data::mbes_ping::PingsT& pings);
std::tuple<ObsT, TransT, AngsT, MatchesT, BBsT, ObsT> create_submaps(const std_data::mbes_ping::PingsT& pings);

void divide_tracks_adaptively(std_data::mbes_ping::PingsT& pings);
double compute_info_in_submap(std_data::mbes_ping::PingsT &submap_pings);
void save_submaps_files(const std_data::mbes_ping::PingsT& pings, const boost::filesystem::path &folder);
}

namespace std_data {

template <>
mbes_ping::PingsT parse_file(const boost::filesystem::path& file);

template <>
nav_entry::EntriesT parse_file(const boost::filesystem::path& file);

}

#endif // NAVI_DATA_H
