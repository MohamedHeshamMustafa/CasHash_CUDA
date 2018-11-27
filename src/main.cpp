#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#include "KeyFileReader.h"
#include "HashConverter.h"
#include "HashMatcher.h"
#include <boost/filesystem.hpp>


int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <list.txt> outfile\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    KeyFileReader keyFileReader;
    keyFileReader.OpenKeyList(argv[1]);
    keyFileReader.ZeroMeanProc();

    std::cerr << "Initializing CUDA device...\n";
    cudaSetDevice(0);

    HashConverter hashConverter;
    HashMatcher hashMatcher;

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
	std::string delimeter = "\\";
	std::string output_dir_name = argv[2] + delimeter + "output.txt";
    FILE *outFile = fopen(output_dir_name.c_str(), "w");

	std::cout << "Output file is " << output_dir_name << '\n';


	//boost::filesystem::path output_dir(output_dir_name);
	//if (boost::filesystem::create_directory(output_dir)) {
	//	std::cout << "Output directory created" << std::endl;
	//}

    cudaEventRecord(start);

    for(int imageIndex = 0; imageIndex < keyFileReader.cntImage; imageIndex++) {
        ImageDevice newImage;	

        std::cerr << "---------------------\nUploading image #" << imageIndex << " to GPU...\n";
        cudaEvent_t kfFinishEvent = keyFileReader.UploadImageAsync(newImage, imageIndex);

        std::cerr << "Calculating compressed Hash Values for image #" << imageIndex << "\n"; 
        cudaEvent_t hcFinishEvent = hashConverter.CalcHashValuesAsync(newImage, kfFinishEvent);

        std::cout << "Matching image #" << imageIndex << " with previous images...\n";
        hashMatcher.AddImageAsync(newImage, hcFinishEvent);

		std::cout << "Before:\n";
        for(int imageIndex2 = 0; imageIndex2 < imageIndex; imageIndex2++) {
			std::cout << "\n\t" << imageIndex << '\n';


            MatchPairListPtr mpList = hashMatcher.MatchPairList(imageIndex, imageIndex2);
            int pairCount = hashMatcher.NumberOfMatch(imageIndex, imageIndex2);

			std::cout << "imageIndex2 " << pairCount << "\n";

            fprintf(outFile, "%d %d\n%d\n", imageIndex2, imageIndex, pairCount);

            for(MatchPairList_t::iterator it = mpList->begin(); it != mpList->end(); it++) {
                fprintf(outFile, "%d %d\n", it->second, it->first);
            }
        }
		std::cout << "After:\n";

    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float timeElapsed;
    cudaEventElapsedTime(&timeElapsed, start, stop);
    std::cerr << "Time elapsed: " << timeElapsed << " ms\n";

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    fclose(outFile);

    return 0;
}
