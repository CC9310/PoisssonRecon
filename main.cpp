#include "PoissonRecon/PoissonRecon.h"
#include "PoissonRecon/SurfaceTrimmer.h"
#include <iostream>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

#define OPENMP_ENABLED

namespace OPT
{
    string input_dir;
    string output_dir;
}

struct PoissonMeshingOptions
{
    // This floating point value specifies the importance that interpolation of
    // the point samples is given in the formulation of the screened Poisson
    // equation. The results of the original (unscreened) Poisson Reconstruction
    // can be obtained by setting this value to 0.
    double point_weight = 1.0;

    // This integer is the maximum depth of the tree that will be used for surface
    // reconstruction. Running at depth d corresponds to solving on a voxel grid
    // whose resolution is no larger than 2^d x 2^d x 2^d. Note that since the
    // reconstructor adapts the octree to the sampling density, the specified
    // reconstruction depth is only an upper bound.
    int depth = 13;

    // If specified, the reconstruction code assumes that the input is equipped
    // with colors and will extrapolate the color values to the vertices of the
    // reconstructed mesh. The floating point value specifies the relative
    // importance of finer color estimates over lower ones.
    double color = 32.0;

    // This floating point values specifies the value for mesh trimming. The
    // subset of the mesh with signal value less than the trim value is discarded.
    double trim = 10.0; //越大裁掉的越shao

    // The number of threads used for the Poisson reconstruction.
    int num_threads = -1;
};

bool PoissonMeshing(const PoissonMeshingOptions &options, const std::string &input_path, const std::string &output_path)
{

    std::vector<std::string> args;

    args.push_back("./binary");
    args.push_back("--in");
    args.push_back(input_path);
    args.push_back("--out");
    args.push_back(output_path);
    args.push_back("--pointWeight");
    args.push_back(std::to_string(options.point_weight));
    args.push_back("--depth");
    args.push_back(std::to_string(options.depth));

    if (options.color > 0)
    {
        args.push_back("--color");
        args.push_back(std::to_string(options.color));
    }

#ifdef OPENMP_ENABLED
    if (options.num_threads > 0)
    {
        args.push_back("--threads");
        args.push_back(std::to_string(options.num_threads));
    }
#endif // OPENMP_ENABLED

    if (options.trim > 0)
    {
        args.push_back("--density");
    }

    std::vector<const char *> args_cstr;
    args_cstr.reserve(args.size());
    for (const auto &arg : args)
    {
        args_cstr.push_back(arg.c_str());
    }

    if (PoissonRecon(args_cstr.size(), const_cast<char **>(args_cstr.data())) != EXIT_SUCCESS)
    {
        return false;
    }

    if (options.trim == 0)
    {
        return true;
    }

    // Trimmer
    args.clear();
    args_cstr.clear();
    args.push_back("./binary");
    args.push_back("--in");
    args.push_back(output_path);
    args.push_back("--out");
    args.push_back(output_path);
    args.push_back("--trim");
    args.push_back(std::to_string(options.trim));

    args_cstr.reserve(args.size());
    for (const auto &arg : args)
    {
        args_cstr.push_back(arg.c_str());
    }

    return SurfaceTrimmer(args_cstr.size(), const_cast<char **>(args_cstr.data())) == EXIT_SUCCESS;
}

void IterateTypeFiles(string pathName, vector<string> &fileNames, string ext)
{
    fileNames.clear();
    if (!boost::filesystem::exists(pathName))
        return;
    boost::filesystem::directory_iterator endIter;
    for (boost::filesystem::directory_iterator iter(pathName); iter != endIter; ++iter)
    {
        if (boost::filesystem::is_regular_file(iter->status()))
        {
            std::string type = iter->path().extension().string();
            transform(type.begin(), type.end(), type.begin(), (int (*)(int))tolower);
            if (type == ext)
                fileNames.push_back(iter->path().filename().string());
        }
    }
    sort(fileNames.begin(), fileNames.end(), std::less<std::string>());
}
int main(int argc, char *argv[])
{

    PoissonMeshingOptions options;
    //    options.color = -1.0;
    options.depth = 8;

    boost::program_options::options_description config("Options");
    config.add_options()("input_dir", boost::program_options::value<string>(&OPT::input_dir), "input path")
                        ("output_dir", boost::program_options::value<string>(&OPT::output_dir), "output path")
                        ("depth", boost::program_options::value<int>(&options.depth), "depth")
                        ("color", boost::program_options::value<double>(&options.color), "color")
                        ("trim", boost::program_options::value<double>(&options.trim), "trim")
                        ("num_threads", boost::program_options::value<int>(&options.num_threads), "num_threads");
    boost::program_options::variables_map vm;
    if (argc == 2)
    {
        cout << argv[1] << endl;
        std::string configFile = argv[1];
        std::ifstream ifs(configFile);
        if (!ifs)
        {
            cout << "Cannot open configfile: " << configFile << endl;
            return 1;
        }
        try
        {
            boost::program_options::store(boost::program_options::parse_config_file(ifs, config), vm);
            boost::program_options::notify(vm);
        }
        catch (const std::exception &e)
        {
            cout << e.what() << endl;
            return 1;
        }
    }
    vector<string> filenames;
    IterateTypeFiles(OPT::input_dir, filenames, ".ply");
    for (int i = 0; i < filenames.size(); i++)
    {
        cout << OPT::input_dir + "/" + filenames[i] << " "
             << "PoissonRecon begin!" << endl;
        bool success = PoissonMeshing(options, OPT::input_dir + "/" + filenames[i], OPT::output_dir + "/" + filenames[i]);
        if (success)
        {
            std::cout << "Surface reconstruction by poisson is successful!" << std::endl;
        }
        else
        {
            std::cout << "Surface reconstruction by poisson is failed!" << std::endl;
        }
    }

    return 0;
}
