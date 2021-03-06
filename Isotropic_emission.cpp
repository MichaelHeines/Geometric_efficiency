#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <math.h>
#include <fstream>
#define pi 3.14159265358979323846


// Define the position class; a vector of xy coordinate pairs and their transformation/calculation functions
class position {
    public:
        std::vector<double> x, y;
    
        position(std::vector<double> x_in, std::vector<double> y_in){                               // Constructor
            x = x_in;
            y = y_in;
        }

        void add_vec(std::vector<double> x_diff, std::vector<double> y_diff){                       // Vector addition: Add vector pairs x_diff, y_diff to existing vector
            int size_1 = x.size();
            int size_2 = x_diff.size();

            if (size_1 != size_2){
                std::cerr << "ERROR: vector addtion with different size vectors" << std::endl;
                exit(0);
            }

            for (int i = 0; i < size_1; i++){
                x[i] += x_diff[i];
                y[i] += y_diff[i];
            }
        }

        std::vector<double> calculate_rsq(){                                                        // Calculate r^2 of an existing vector
            int size = x.size();
            std::vector<double> r_sq(size);

            for (int i = 0; i < size; i++){
                r_sq[i] = x[i]*x[i] + y[i]*y[i];
            }
            return r_sq;
        }

        // On empty position (to be filled), generate an isotropic distribution that gives extrapolated changes in x and y directions
        void generate_isotropic(double z, int seed){
            std::default_random_engine generator{seed};
            std::uniform_real_distribution<double> phi_distr(0, 2*pi);
            std::uniform_real_distribution<double> theta_create_distr(0, 1);
            double phi, theta;

            for (int i = 0; i < x.size(); i++){
                phi = phi_distr(generator);
                theta = acos(1 - 2 * theta_create_distr(generator));
                x[i] = z * tan(theta) * cos(phi);
                y[i] = z * tan(theta) * sin(phi);
            }
        }

        // On empty position (to be filled), generate a random point inside a uniform circular source
        void generate_circular_distr(double r_s, int seed){
            std::default_random_engine generator{seed};
            std::uniform_real_distribution<double> phi_distr(0, 2*pi);
            std::uniform_real_distribution<double> r_create_distr(0, 1);
            double phi, r;

            for (int i = 0; i < x.size(); i++){
                phi = phi_distr(generator);
                r = r_s * sqrt(r_create_distr(generator));
                x[i] = r * cos(phi);
                y[i] = r * sin(phi);
            }
        }

        // On empty position (to be filled), generate a random point inside a gaussian circular source
        void generate_gaussian_distr(double sigma, int seed){
            std::default_random_engine generator{seed};
            std::uniform_real_distribution<double> phi_distr(0, 2*pi);
            std::normal_distribution<double> r_distr(0, sigma);
            double phi, r;

            for (int i = 0; i < x.size(); i++){
                phi = phi_distr(generator);
                r = r_distr(generator);
                x[i] = r * cos(phi);
                y[i] = r * sin(phi);
            }
        }

};


// Make a linspace
std::vector<double> linspace(double min, double max, int nr_points){
    std::vector<double> out(nr_points);
    double delta = (max - min)/(1.0 * (nr_points - 1));

    for (int i = 0; i < nr_points; i++){
        out[i] = min + delta*i;
    }
    return out;
}


// Calculate the point source approximation value for the values in vector z
std::vector<double> point_source(std::vector<double> z) {
    int size = z.size();
    std::vector<double> ps(size);
    double temp;

    for (int i = 0; i < size; i++) {
        temp = 50 - (50 * z[i]) / (sqrt(1 + pow(z[i], 2)));
        ps[i] = temp;
    }
    return ps;
}


// Calculate the geometric efficiency at a specific distance
double geom_eff_point(double z, double source, int n, int seed, std::string source_type){
    int N_hit = 0;                                                          // Initialize hit counter
    std::vector<double> x1(n), x2(n), y1(n), y2(n);
    
    position generate_source(x1, y1);
    position generate_emission(x2, y2);
     
    if (source_type == "uniform"){                                          // Generate source position
        generate_source.generate_circular_distr(source, seed);
    } else if (source_type == "gaussian"){
        generate_source.generate_gaussian_distr(source, seed);
    } else{
        std::cerr << "ERROR: Not a valid source type. Choose 'circular' or 'gaussian'" << std::endl;
        exit(0);
    }
    
    seed++;                                                                 // Increment seed to avoid correlated random numbers
    generate_emission.generate_isotropic(z, seed);                          // Extrapolated position at the same distance as the detector
    generate_source.add_vec(generate_emission.x, generate_emission.y);
    std::vector<double> r_final = generate_source.calculate_rsq();          // Calculate r for the extrapolated end position

    // Check if it was a hit or a miss
    for (int i = 0; i < n; i++){
        if (r_final[i] <= 1){
            N_hit++;                                                        // Add 1 to hit counter
        }
    }
    return 50.0*N_hit/n;                                                    // Get hit/emitted percentage, note isotropic distribution is extrapolated to the detector side -> 1/2
}


// Write the output file
void write_geo_file(std::vector<double> z, std::vector<double> efficiencies, std::vector<double> rel_ers, std::string filename) {
    std::vector<double> e_ps = point_source(z);
    std::ofstream myFile(filename);
    myFile << "z/rd \t point source \t Model \t Relative uncertainty \n";

    for (int i = 0; i < z.size(); i++) {
        myFile << z[i] << "\t" << e_ps[i] << "\t" << efficiencies[i] << "\t" << rel_ers[i]<< "\n";
    }
    
    std::cout << "Wrote output file" << std::endl;
    myFile.close();
}


int main(int argc, char **argv){
    int seed = 15763027;                                                    // Randomly picked seed
    std::string source_type, detector_type;
    double z_min, z_max, source, result, det_fraction;
    int n_points, power;
    std::string filename;

    // Check if the arguments were appropriate
    if (argc != 3){
        std::cerr << "ERROR: input option 'uniform' or 'gaussian' for the source distribution and 'circular' or 'annular' for the detector" << std::endl;
        exit(0);
    } else{
        source_type = argv[1];
        detector_type = argv[2];

        if (source_type != "uniform" && source_type != "gaussian"){
            std::cerr << "ERROR: input option 'uniform' or 'gaussian' for the source distribution" << std::endl;
            exit(0);
        }
        else if (detector_type != "circular" && detector_type != "annular"){
            std::cerr << "ERROR: input option 'circular' or 'annular' for the source distribution" << std::endl;
            exit(0);
        }
    }

    // Input values
    std::cout << "z_min/rd:" << std::endl;
    std::cin >> z_min;
    std::cout << "z_max/rd:" << std::endl;
    std::cin >> z_max;
    std::cout << "number of points:" << std::endl;
    std::cin >> n_points;
    std::cout << "source/rd:" << std::endl;
    std::cin >> source;
    std::cout << "Power:" << std::endl;
    std::cin >> power;
    if (detector_type == "annular"){
        std::cout << "Detector outer/inner:" << std::endl;
        std::cin >> det_fraction;
    }
    std::cout << "Filename:" << std::endl;
    std::cin >> filename;

    int n_perpoint = pow(10, power);
    std::vector<double> z = linspace(z_min, z_max, n_points);
    std::vector<double> efficiencies(n_points), rel_ers(n_points), efficiencies_outer(n_points), efficiencies_inner(n_points), rel_ers_outer(n_points), rel_ers_inner(n_points);
    std::cout << "Completion(%)" << "\t" << "Efficiency (%)" << "\t \t" << "Relative error (%)" << std::endl;

    // Calculate the geometric efficiency at all points
    for (int i = 0; i < n_points; i++){
        if (detector_type == "circular"){ 
            efficiencies[i] = geom_eff_point(z[i], source, n_perpoint, seed, source_type);
            rel_ers[i] = 100 / sqrt(2 * n_perpoint*efficiencies[i]/100);
        }
        else if (detector_type == "annular") { 
            efficiencies_outer[i] = geom_eff_point(z[i], source, n_perpoint, seed, source_type);
            efficiencies_inner[i] = geom_eff_point(z[i] * det_fraction, source * det_fraction, n_perpoint, seed, source_type);
            efficiencies[i] = efficiencies_outer[i] - efficiencies_inner[i];
            rel_ers_outer[i] = 100 / sqrt(2 * n_perpoint*efficiencies_outer[i]/100);
            rel_ers_inner[i] = 100 / sqrt(2 * n_perpoint*efficiencies_inner[i]/100);
            rel_ers[i] = sqrt(rel_ers_outer[i] * rel_ers_outer[i] + rel_ers_inner[i] * rel_ers_inner[i]);
        }
        else{
            std::cerr << "ERROR: Detector types can only be circular/annular" << std::endl;
            exit(0);
        }

        std::cout << z[i]/z[n_points-1] << "\t" << efficiencies[i] << "\t \t" << rel_ers[i] << std::endl; 
    }
    
    // Write the output file
    write_geo_file(z, efficiencies, rel_ers, filename);
    return 1;
}
