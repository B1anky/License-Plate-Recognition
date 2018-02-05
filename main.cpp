#include "opencv2/opencv.hpp"
#include <vector>
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::experimental::filesystem;

typedef struct character_match_t
{
	cv::Point2i position;
	cv::Mat image;
} character_match_t;

/*This function searches the current windows directory given by val
and gets the license plate files to test with*/
std::vector<std::string> getTestingFiles(std::string val)
{
	std::vector<std::string> result;

	std::string path = val;
	for (auto & p : fs::directory_iterator(path)) {
		result.push_back(p.path().string());
	}
	return result;
}

/*This function searches the current windows directory given by val
and gets the characters to train with*/
std::vector<std::string> getTrainingFiles(std::string val)
{
	std::vector<std::string> result;

	std::string path = val;
	for (auto & p : fs::directory_iterator(path)) {
		result.push_back(p.path().string());
	}
	return result;
}

/*This function takes a license plate images and resizes, 
equalizes, thresholds, and erodes with a 3x3 kernel*/
cv::Mat EqualizeAndErode(cv::Mat const& img)
{
	cv::Mat resized_img;
	cv::resize(img, resized_img, cv::Size(1920, 1200));

	cv::Mat equalized_img;
	cv::equalizeHist(resized_img, equalized_img);

	cv::Mat final_img = equalized_img;

	cv::Mat mask;
	cv::threshold(final_img, mask, 45, 255, cv::THRESH_BINARY);

	cv::Mat kernel(cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
	cv::erode(mask, mask, kernel, cv::Point(-1, -1), 1);

	return mask;
}

/*This function does the same as above, but changes the threshold from 45 to 210*/
cv::Mat IEqualizeAndErode(cv::Mat const& img)
{
	cv::Mat resized_img;
	cv::resize(img, resized_img, cv::Size(1920, 1200));

	cv::Mat equalized_img;
	cv::equalizeHist(resized_img, equalized_img);

	cv::Mat final_img = equalized_img;

	cv::Mat mask;
	cv::threshold(final_img, mask, 210, 255, cv::THRESH_BINARY);

	cv::Mat kernel(cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
	cv::erode(mask, mask, kernel, cv::Point(-1, -1), 1);

	return mask;
}

cv::Point2i Center(cv::Rect const& bounding_box)
{
	return cv::Point2i(bounding_box.x + bounding_box.width / 2, bounding_box.y + bounding_box.height / 2);
}

bool compareCharMatch(character_match_t a, character_match_t b) {
	return a.position.x < b.position.x;
}

/*This function find the locations of pssible characters on a testing plate*/
std::vector<character_match_t> ExtractCharacters(cv::Mat const& img)
{
	cv::Mat inversedImg;
	cv::bitwise_not(img, inversedImg);

	cv::imshow("InversedImg", inversedImg);
	cv::waitKey(0);

	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;

	cv::findContours(inversedImg, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	std::vector<character_match_t> result;

	//These constraints are used to limit character candidacy
	double const MIN_CONTOUR_AREA = 25000.0;
	double const MAX_CONTOUR_AREA = 250000.0;
	double const MAX_CONTOUR_COL1 = 150.0;
	double const MAX_CONTOUR_COL2 = 1800.0;
	double const MAX_CONTOUR_ROW = 685.0;

	for (int i = 0; i < contours.size(); i++) {
		cv::Rect bounding_box(cv::boundingRect(contours[i]));
		int area = bounding_box.area();
		if ((area >= MIN_CONTOUR_AREA) && (area <= MAX_CONTOUR_AREA)){
			//Ignore contours which do not reside within the middle of the possible pixel regions
			bool isValid = false;
			//x is column, y is row
			for (int j = 0; j < contours[i].size(); j++) {
				//685 is more or less the center line by height and 150 to 1850 the bounding width for candidates
				if ((contours[i][j].y == 685 && (contours[i][j].x > 150 && contours[i][j].x < 1850))) {

					isValid = true;
					break;
				}
			}

			int PADDING = 2;
			bounding_box.x -= PADDING;
			bounding_box.y -= PADDING;
			bounding_box.width += PADDING * 2;
			bounding_box.height += PADDING * 2;

			character_match_t match;
			if (bounding_box.x > 0 && bounding_box.y > 0 && bounding_box.x + bounding_box.width < img.cols && bounding_box.y + bounding_box.height < img.rows) {
				match.position = Center(bounding_box);
				match.image = img(bounding_box);
				if (isValid) {
					result.push_back(match);
					cv::imshow("Character", match.image);
					cv::waitKey(0);
				}
			}
		}
	}

	//This sort just makes the characters read out left to right when being displayed
	std::sort(result.begin(), result.end(), compareCharMatch);

	return result;
}

/*This function does the same as above, but for the inverted plates*/
std::vector<character_match_t> IExtractCharacters(cv::Mat const& img)
{
	cv::Mat inversedImg;
	cv::bitwise_not(img, inversedImg);

	cv::imshow("InversedImg", img);
	cv::waitKey(0);

	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;

	cv::findContours(img, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	std::vector<character_match_t> result;

	double const MIN_CONTOUR_AREA = 45000.0;
	double const MAX_CONTOUR_AREA = 250000.0;
	double const MAX_CONTOUR_COL1 = 150.0;
	double const MAX_CONTOUR_COL2 = 1800.0;
	double const MAX_CONTOUR_ROW = 685.0;

	for (int i = 0; i < contours.size(); i++) {
		cv::Rect bounding_box(cv::boundingRect(contours[i]));
		int area = bounding_box.area();
		if ((area >= MIN_CONTOUR_AREA) && (area <= MAX_CONTOUR_AREA)) {
			//Ignore contours which do not reside within the middle of the possible pixel regions
			bool isValid = false;
			//x is column, y is row
			for (int j = 0; j < contours[i].size(); j++) {
				//685 is more or less the center line by height and 150 to 1850 the bounding width for candidates
				if ((contours[i][j].y == 685 && (contours[i][j].x > 150 && contours[i][j].x < 1850))) {

					isValid = true;
					break;
				}
			}

			int PADDING = 2;
			bounding_box.x -= PADDING;
			bounding_box.y -= PADDING;
			bounding_box.width += PADDING * 2;
			bounding_box.height += PADDING * 2;

			character_match_t match;
			if (bounding_box.x > 0 && bounding_box.y > 0 && bounding_box.x + bounding_box.width < img.cols && bounding_box.y + bounding_box.height < img.rows) {
				match.position = Center(bounding_box);
				match.image = inversedImg(bounding_box);
				if (isValid) {
					result.push_back(match);
					cv::imshow("Character", match.image);
					cv::waitKey(0);
				}
			}
		}
	}

	//This sort just makes the characters read out left to right when being displayed
	std::sort(result.begin(), result.end(), compareCharMatch);

	return result;
}

/*This function takes a character and resizes it to 25x25 and applies thresholding*/
std::pair<float, cv::Mat> TrainCharacter(char c, cv::Mat const& img)
{
	cv::Mat small_char;
	cv::resize(img, small_char, cv::Size(25, 25), 0, 0, cv::INTER_LINEAR);
	cv::threshold(small_char, small_char, 64, 255, cv::THRESH_BINARY);

	cv::Mat small_char_float;
	small_char.convertTo(small_char_float, CV_32FC1);

	cv::Mat small_char_linear(small_char_float.reshape(1, 1));

	return std::pair<float, cv::Mat>(static_cast<float>(c), small_char_linear);
}

/*Main function to start subsequent identification based on plate image and the trained data points.
It returns the string for the processed img*/
std::string ProcessImage(cv::Mat const& img, cv::Ptr<cv::ml::KNearest> knn)
{
	cv::Mat clean_img = EqualizeAndErode(img);
	std::vector<character_match_t> characters(ExtractCharacters(clean_img));
	
	if (characters.empty()) {
		clean_img = IEqualizeAndErode(img);
		characters = IExtractCharacters(clean_img);
	}

	std::string result;
	for (character_match_t const& match : characters) {
		cv::Mat small_char;
		cv::resize(match.image, small_char, cv::Size(25, 25), 0, 0, cv::INTER_LINEAR);

		cv::Mat small_char_float;
		small_char.convertTo(small_char_float, CV_32FC1);
		cv::imshow("Small_char", small_char_float);
		cv::waitKey(0);

		cv::Mat small_char_linear(small_char_float.reshape(1, 1));

		cv::Mat temp;

		float p = knn->findNearest(small_char_linear, 1, temp);

		result.push_back(char(p));
	}

	return result;
}

/*Driver for the program
First, it trains the knn with the characters directory
Second, it reads the plates in and applies the processImage on each
Finally, it displays the results on the terminal of each processed plate*/
int main()
{
	std::vector<std::string> train_files(getTrainingFiles("./Chars/"));
	
	cv::Mat samples, responses;
	for (std::string const& file_name : train_files) {
		std::cout << file_name << std::endl;
		cv::Mat char_img(cv::imread(file_name, 0));
		std::pair<float, cv::Mat> trainingInfo(TrainCharacter(file_name[file_name.size() - 5], char_img));
		responses.push_back(trainingInfo.first);
		samples.push_back(trainingInfo.second);
	}

	cv::Ptr<cv::ml::KNearest> knn(cv::ml::KNearest::create());
	cv::Ptr<cv::ml::TrainData> trainingData = cv::ml::TrainData::create(samples, cv::ml::SampleTypes::ROW_SAMPLE, responses);

	knn->train(trainingData);

	std::vector<std::string> input_files(getTestingFiles("./Plates/"));

	for (std::string const& file_name : input_files) {
		cv::Mat plate_img = cv::imread(file_name, 0);
		std::string plate(ProcessImage(plate_img, knn));
		std::cout << file_name << " : " << plate << "\n";
	}
	system("Pause");
}