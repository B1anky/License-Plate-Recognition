To run the program, OpenCV needed to be modified because C++17 features were used and the random.h of OpenCV has the deprecated
function called randum_shuffle() since C++14. In order for it to run with C++17, those lines which cause the error can be temporarily 
commented out in order to allow compilation. Just run the program and it should find all of the files in their corresponding folder without
any work on your end. To run through the program and see each step, hit or press any key on the keyboard / hold a key to go through it more
quickly. When no more windows seem to be opening is how you can tell the program has finished processing and the terminal will contain
the information of the results that should match what is in the paper. (I reordered the results in the paper because the program runs through the plates lexicographically and not numerically).