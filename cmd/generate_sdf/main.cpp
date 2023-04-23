
#include <Discregrid/All>
#include <Eigen/Dense>

#include "resource_path.hpp"

#include <string>
#include <iostream>
#include <array>

#include<fstream>
#include<rapidjson/prettywriter.h>
#include<rapidjson/document.h>

using namespace Eigen;

std::istream& operator>>(std::istream& is, std::array<unsigned int, 3>& data)  
{  
	is >> data[0] >> data[1] >> data[2];  
	return is;  
}  

std::istream& operator>>(std::istream& is, AlignedBox3d& data)  
{  
	is	>> data.min()[0] >> data.min()[1] >> data.min()[2]
		>> data.max()[0] >> data.max()[1] >> data.max()[2];  
	return is;  
}  

#include <cxxopts/cxxopts.hpp>


//void LoadMeshFromGLTF(const std::string const& path, std::vector<Eigen::Vector3d>& vertices, std::vector<std::array<unsigned int, 3>>& faces)
//{
//	GLTFLoader loader;
//
//	loader.LoadContent(path);
//
//	const auto& doc = loader.mDoc;
//	//get mesh
//	if (doc.HasMember("meshes"))
//	{
//		const rapidjson::Value& meshes = doc["meshes"];
//		for (int i=0;i<meshes.Size();i++)
//		{
//			// mesh name
//			std::string meshName = loader.getMeshName(meshes, i);
//
//			const rapidjson::Value& mesh = meshes[i];
//
//			// mesh primitives
//			auto itMeshPrimitives = mesh.FindMember("primitives");
//			if (itMeshPrimitives != mesh.MemberEnd())
//			{
//
//			}
//		}
//	}
//}

void LoadModel(const std::string const& path, std::vector<Eigen::Vector3d>& vertices, std::vector<std::array<unsigned int, 3>>& faces)
{
	std::ifstream in(path, std::ios::in);
	if (!in)
	{
		std::cerr << "Cannot open " << path << std::endl;
		return;
	}

	auto lastindex = path.find_last_of(".");
	std::string extension = path.substr(lastindex + 1, path.length() - lastindex);
	if (extension == "obj")
	{
		std::string line;
		std::vector<unsigned int> faceIndices; //all indices of face in a line
		while (getline(in, line)) {
			if (line.substr(0, 2) == "v ") {
				std::istringstream s(line.substr(2));
				Vector3d v; s >> v.x(); s >> v.y(); s >> v.z();
				vertices.push_back(v);
			}
			else if (line.substr(0, 2) == "f ") {
				/*std::istringstream s(line.substr(2));
				std::array<unsigned int, 4> f;
				for (unsigned int j(0); j < 4; ++j)
				{
					std::string buf;
					s >> buf;
					buf = buf.substr(0, buf.find_first_of('/'));
					f[j] = std::stoi(buf) - 1;
				}
				
				faces.push_back({ f[0], f[1], f[2] });
				faces.push_back({f[0], f[2], f[3]});*/
				//////////////*hard code, to be updated by using tiny_obj*/////////////
				std::istringstream s(line.substr(2));
				std::string buf;
				while (s >> buf)
				{
					buf = buf.substr(0, buf.find_first_of('/'));
					faceIndices.emplace_back(std::stoi(buf) - 1);
				}
				faces.push_back({ faceIndices[0], faceIndices[1], faceIndices[2] });
				if(faceIndices.size() > 3)
					faces.push_back({ faceIndices[0], faceIndices[2], faceIndices[3] });
				faceIndices.clear();
			}
			else if (line[0] == '#') { /* ignoring this line */ }
			else { /* ignoring this line */ }
		}
	}
	else if ("gltf")
	{
		//LoadMeshFromGLTF(path, vertices, faces);
	}
}

int main(int argc, char* argv[])
{
	cxxopts::Options options(argv[0], "Generates a signed distance field from a closed two-manifold triangle mesh.");
	options.positional_help("[input OBJ file]");

	options.add_options()
	("h,help", "Prints this help text")
	("r,resolution", "Grid resolution", cxxopts::value<std::array<unsigned int, 3>>()->default_value("10 10 10"))
	("d,domain", "Domain extents (bounding box), format: \"minX minY minZ maxX maxY maxZ\"", cxxopts::value<AlignedBox3d>())
	("i,invert", "Invert SDF")
	("o,output", "Ouput file in cdf format", cxxopts::value<std::string>()->default_value(""))
	("input", "OBJ file containing input triangle mesh", cxxopts::value<std::vector<std::string>>())
	;

	try
	{
		options.parse_positional("input");
		auto result = options.parse(argc, argv);

		//if (result.count("help"))
		//{
		//	std::cout << options.help() << std::endl;
		//	std::cout << std::endl << std::endl << "Example: GenerateSDF -r \"50 50 50\" SimpleHouse.obj" << std::endl;
		//	exit(0);
		//}
		//if (!result.count("input"))
		//{
		//	std::cout << "ERROR: No input mesh given." << std::endl;
		//	std::cout << options.help() << std::endl;
		//	std::cout << std::endl << std::endl << "Example: GenerateSDF -r \"50 50 50\" dragon.obj" << std::endl;
		//	exit(1);
		//}
		auto resolution =  std::array<unsigned int, 3>({ 30,30,30 });//result["r"].as<std::array<unsigned int, 3>>();
		//auto filename = "D:/GritWorld-Shanghai-WorkSpace/Work_Tasks/Feature_SDF/Discregrid/cmd/generate_sdf/resources/bunny.obj";// result["input"].as<std::vector<std::string>>().front(); 
		auto filename = "D:/GritWorld-Shanghai-WorkSpace/Work_Tasks/Feature_SDF/ModelData/FromBlender/obj/SDFScene/SDF2BoxesSphere.obj";
		if (!std::ifstream(filename).good())
		{
			std::cerr << "ERROR: Input file does not exist!" << std::endl;
			//exit(1);
		}

		std::cout << "Load mesh...";
		std::vector<Eigen::Vector3d> vertices; 
		std::vector<std::array<unsigned int, 3>> faces;
		LoadModel(filename, vertices, faces);
		Discregrid::TriangleMesh mesh(vertices, faces);
		std::cout << "DONE" << std::endl;

		std::cout << "Set up data structures...";
		Discregrid::TriangleMeshDistance md(mesh);
		std::cout << "DONE" << std::endl;

		Eigen::AlignedBox3d domain;
		domain.setEmpty();
		if (result.count("d"))
		{
			domain = result["d"].as<Eigen::AlignedBox3d>();
		}
		if (domain.isEmpty())
		{
			for (auto const& x : mesh.vertices())
			{
				domain.extend(x);
			}

			//need to expand a little
			/*domain.max() += (1.0e-3 * domain.diagonal().norm() * Vector3d::Ones())+ Vector3d(0.1, 0.1, 0.1);
			domain.min() -= (1.0e-3 * domain.diagonal().norm() * Vector3d::Ones())+ Vector3d(0.1, 0.1, 0.1);*/
		}

		Discregrid::CubicLagrangeDiscreteGrid sdf(domain, resolution);
		auto func = Discregrid::DiscreteGrid::ContinuousFunction{};
		if (result.count("invert"))
		{
			func = [&md](Vector3d const& xi) {return -1.0 * md.signed_distance(xi).distance; };
		}
		else
		{
			func = [&md](Vector3d const& xi) {return md.signed_distance(xi).distance; };
		}

		std::cout << "Generate discretization..." << std::endl;
		sdf.addFunction(func, true);
		std::cout << "DONE" << std::endl;

		sdf.CalculateSDFBuffer();
		std::cout << "Serialize discretization...";
		auto output_file = result["o"].as<std::string>();
		auto output_Customerize_file = result["o"].as<std::string>();
		auto output_Customerize_file2 = result["o"].as<std::string>();
		auto output_SDF_file = result["o"].as<std::string>();
		auto output_SDF_Tex_file = result["o"].as<std::string>();
		if (output_file == "")
		{
			output_file = filename;
			output_Customerize_file = filename;
			output_SDF_file = filename;
			output_Customerize_file2 = filename;
			output_SDF_Tex_file = filename;
			if (output_file.find(".") != std::string::npos)
			{
				auto lastindex = output_file.find_last_of(".");
				output_file = output_file.substr(0, lastindex);
				output_Customerize_file = output_file.substr(0, lastindex);
				output_SDF_file = output_file.substr(0, lastindex);
				output_Customerize_file2 = output_file.substr(0, lastindex);
				output_SDF_Tex_file = output_file.substr(0, lastindex);
			}
			output_file += ".cdf";
			output_Customerize_file += ".cudf";
			output_SDF_file += ".sdf";
			output_Customerize_file2 += ".cudf2";
			output_SDF_Tex_file += ".sdft";
		}
		sdf.save(output_file);

		/*sdf.saveCustomerize(output_Customerize_file);
		sdf.saveCustomerize2(output_Customerize_file2);
		sdf.saveSDF(output_SDF_file);*/

		sdf.saveSDFTexture(output_SDF_Tex_file);
		std::cout << "DONE" << std::endl;
	}
	catch (cxxopts::OptionException const& e)
	{
		std::cout << "error parsing options: " << e.what() << std::endl;
		exit(1);
	}
	
	return 0;
}