# gpgs_slam
Gaussian Process Graph Slam Multibeam SLAM System

## Dependencies

On Ubuntu 16.04, just use the following command to install all dependencies:
```
sudo apt-get install libcereal-dev libglfw3-dev libceres-dev
```

## Building

Once cloned, get the libigl submodule via `git submodule init`, `git submodule update`.
Then, create a build folder, and run `cmake ../src`, followed by `make` within that folder.

## Running

You can run a toy example with data provided in this repo.
In the `scripts` folder, execute `./generate_submaps.py`. This creates
a bunch of data files within the `scripts` folder. Next, go to your
build folder and run
```
./parse_sim_data --folder ../scripts --file example_problem.cereal
```
This creates the optimization problem `example_problem.cereal`. You can view it with
```
./visualize_slam_process --file example_problem.cereal
```
Now, we can optimize it by running
```
./slam_process_ceres --file example_problem.cereal --output example_results.cereal
```
Again, the results `example_results.cereal` can be viewed using the visualizer.

## data_tools

### navi_data

The `navi_data` library contains tools for parsing files exported from NaviEdit.
See this example program, also available in [the test folder](https://github.com/nilsbore/gpgs_slam/blob/master/test/test_parse_navi_data.cpp).

```cpp
#include <boost/filesystem.hpp>
#include <cxxopts.hpp>
#include <data_tools/data_structures.h>
#include <data_tools/navi_data.h>
#include <gpgs_slam/transforms.h>

using namespace std;

int main(int argc, char** argv)
{
    string folder_str;

    cxxopts::Options options("MyProgram", "One line description of MyProgram");
    options.add_options()("help", "Print help")("folder", "Input folder", cxxopts::value(folder_str));

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        cout << options.help({ "", "Group" }) << endl;
        exit(0);
    }

    boost::filesystem::path folder(folder_str);
    boost::filesystem::path pings_path = folder / "mbes_pings.cereal";
    boost::filesystem::path entries_path = folder / "nav_entries.cereal";
    boost::filesystem::path submaps_path = folder / "submaps.cereal";

    // parse the files from the folder
    mbes_ping::PingsT pings = parse_folder<mbes_ping>(folder / "Pings");
    nav_entry::EntriesT entries = parse_folder<nav_entry>(folder / "NavUTM");

    // match timestamps of pings and nav entries
    match_timestamps(pings, entries);

    // Divides the tracks into roughly square pieces
    divide_tracks_equal(pings);
    //divide_tracks(pings);

    // write to disk
    write_data(pings, pings_path);
    write_data(entries, entries_path);

    // read from disk
    pings = read_data<mbes_ping::PingsT>(pings_path);
    entries = read_data<nav_entry::EntriesT>(entries_path);

    // convert to submaps
    pt_submaps ss;
    tie(ss.points, ss.trans, ss.angles, ss.matches, ss.bounds) = create_submaps(pings);
    for (const Eigen::Vector3d& ang : ss.angles) {
        ss.rots.push_back(euler_to_matrix(ang(0), ang(1), ang(2)));
    }

    // write to disk
    write_data(ss, submaps_path);
    // read from disk
    ss = read_data<pt_submaps>(submaps_path);

    return 0;
}
```
