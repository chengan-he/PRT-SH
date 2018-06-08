#define _USE_MATH_DEFINES
#include <cmath>
#include <glm/glm.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "brdf.h"
#include "sampler.h"

Sampler viewSampler(SAMPLE_NUMBER);
std::string band_name[] = {"constant", "linear", "quadratic", "cubic", "quartic"};

void BRDF::init(int band, BRDF_TYPE type)
{
    int band2 = band * band;
    Sampler lightSampler(sampleNumber);
    lightSampler.computeSH(band);
    int lightSampleNumber = lightSampler._samples.size();
    float weight = 4.0f * M_PI / lightSampleNumber;
    _BRDFlookupTable = new Eigen::VectorXf*[sampleNumber];

    for (int i = 0; i < sampleNumber; i++)
    {
        _BRDFlookupTable[i] = new Eigen::VectorXf[sampleNumber];
        for (int j = 0; j < sampleNumber; j++)
        {
            _BRDFlookupTable[i][j].resize(band2);
            _BRDFlookupTable[i][j].setZero();

            if (type == Phong)
            {
                // The naive version of Phong, ignoring spatial variance.
                Sample vsp = viewSampler._samples[i * sampleNumber + j];
                glm::vec3 normal(0.0f, 1.0f, 0.0f);
                const float diffuse_albedo = 1.2f;
                const int shininess = 4.0f;

                // Monte-Carlo integration for light directions.
                for (int k = 0; k < lightSampleNumber; k++)
                {
                    Sample lsp = lightSampler._samples[k];
                    // Naive phong.
                    glm::vec3 reflect = 2 * glm::dot(normal, lsp._cartesCoord) * normal - lsp._cartesCoord;
                    float specular = std::max(glm::dot(glm::normalize(reflect), glm::normalize(vsp._cartesCoord)),
                                              0.0f);
                    float brdf = diffuse_albedo / M_PI + powf(specular, shininess);
                    // Projection.
                    for (int l = 0; l < band2; l++)
                    {
                        _BRDFlookupTable[i][j](l) += lsp._SHvalue[l] * brdf * std::max(0.0f, lsp._cartesCoord.z);
                    }
                }
            }
            // Normalization.
            for (int k = 0; k < band2; k++)
            {
                _BRDFlookupTable[i][j](k) = _BRDFlookupTable[i][j](k) * weight;
            }
        }
    }

#ifdef SHOW_BRDF
    cv::Mat brdf(sampleNumber * 2, sampleNumber * 2, CV_32FC1);
    for (int i = 0; i < sampleNumber * 2; i++)
    {
        for (int j = 0; j < sampleNumber * 2; j++)
        {
            brdf.at<float>(i * sampleNumber * 2 + j) = _BRDFlookupTable[i / 2][j / 2].squaredNorm();
        }
    }
    cv::Mat1b brdf_8UC1;
    brdf.convertTo(brdf_8UC1, CV_8UC1, 255);
    // cv::imshow("brdf", brdf_8UC1);
    switch (type)
    {
    case Phong:
        cv::imwrite("brdf/phong_" + band_name[band - 1] + ".jpg", brdf_8UC1);
        break;
    case AS:
        cv::imwrite("brdf/AS_" + band_name[band - 1] + ".jpg", brdf_8UC1);
        break;
    case PF:
        cv::imwrite("brdf/PF_" + band_name[band - 1] + ".jpg", brdf_8UC1);
        break;
    default:
        break;
    }
    cvWaitKey(0);
#endif
}
