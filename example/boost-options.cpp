#include<iostream>
#include<boost/program_options.hpp>

using namespace std;

int main(int argc, char **argv)
{

  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()("help", "describe arguments")("flag", po::value<int>(), "flag");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  if (vm.count("flag")) {
    cout << "flag is set to " << vm["flag"].as<int>() << ".\n";
  }
  else {
    cout << "flag not set.\n";
  }
}

template<class C> C::value_type sum(const C &a)
{
  return accumulate(a.begin(), a.end(), 0);
}

array<int, 10> a10;
array<double, 1000> a1000;
vector<int> v;
// ...
int x1 = sum(a10);
double x2 = sum(a1000);
int x3 = sum(v);