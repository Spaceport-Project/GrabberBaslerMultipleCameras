
/*
 * Copyright (C) 2010-2022 TunaCode Pvt. Ltd.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 *
 * TUNACODE MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED
 * WARRANTY OF ANY KIND.  TUNACODE DISCLAIMS ALL WARRANTIES WITH REGARD TO`
 * THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL
 * TUNACODE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOURCE CODE.
 * 
 * 
 * This file demo the capability of demosaic/debayer on cuvi with three examples:
 * 
 * demosaic1(): Inputs an old-film scanned image with 16 bit data in 16 bit container. A fairly simple demosaic example
 * 
 * 
 * demosaic2(): The input image is an 8 bit image in a 16 bit container with an offset of 100 i.e. its data range is 100-355
 * 
 * 
 * demosaic3(): 
 *	A detailed example of demosaic. 
 *
 *	Input: A 12-bit CFA image in a 16 bit container.
 * 	Output 1: An 8-bit RGB image.
 * 	Output 2: An 8-bit UYVY image but half the width and height of the original image.
 *
 * 	In this example we demostrate how to feed data to CuviImage from a pointer and the result from CuviImage back to the pointer.
 * 	Also we go into details of using various parameters while using cuvi features.
 *
 	
 */


#include <cuvi.h>
#include <fstream>
#include <iostream>
#include <cstring> 


using namespace std;


void demosaic1()
{
	//Inputs an old-film scanned image with 16 bit data in 16 bit container. A fairly simple demosaic example

	string ipath = "sample1.tif";
	
	string opath = ipath + "_results.tiff";

	CuviStatus st;
	CuviImage input, output;


	st = input.create(ipath, CUVI_LOAD_IMAGE_GRAYSCALE_KEEP_DEPTH);
	if(CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }

	//Perform Demosaic
	st = cuvi::colorOperations::demosaicDFPD(input, output, CuviBayerSeq::CUVI_BAYER_RGGB);
	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }

	// Save results to disk
	st = cuvi::io::saveImage(output, opath);
	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }

    cout << "\nProgram 1 executed successfully.." << endl;

	return;
}

 
 void demosaic2(){

 	/** The input image is an 8 bit image in a 16 bit container with an offset of 100 i.e. its data range is 100-355 **/

 	string ipath = "datasets/demosaic_1500x1000_8bitin16_RGGB_offset100_100-355.tif";
 	string opath = ipath + "results.tif";

 	CuviStatus st;
 	CuviImage input, output;
 	CuviBayerSeq sensorAlignment = CuviBayerSeq::CUVI_BAYER_RGGB;

	
 	st = input.create(ipath, CUVI_LOAD_IMAGE_GRAYSCALE_KEEP_DEPTH);
 	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }



 	// removing the offset of 100. Setting data to 0-255 from 100-355
 	/* input -= 100; //Can be done like this but it's better to catch the status */
 	st = cuvi::arithmeticLogical::add(input, -100.0, input);
 	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }


 	//Convert image from 16-bit to 8-bit since our data is now 0-255
 	st = input.convertTo(input, CUVI_8UC1);
 	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }


 	//Perform Demosaic
 	st = cuvi::colorOperations::demosaicDFPD(input, output, sensorAlignment);
 	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }


 	//Save result to disk
 	st = cuvi::io::saveImage(output, opath);
 	if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }

    cout << "\nProgram 2 executed successfully.." << endl;
    return;
 }


 void demosaic3(){

 	/** 
 	A detailed example of demosaic. 
	
 	Input: A 12-bit CFA image in a 16 bit container.
 	Output 1: An 8-bit RGB image.
 	Output 2: An 8-bit UYVY image but half the width and height of the original image.
	
 	In this example we demostrate how to feed data to CuviImage from a pointer and the result from CuviImage back to the pointer.
 	Also we go into details of using various parameters while using cuvi features.

 	**/

 	/*************************User Parameters*************************/

 	string ipath = "datasets/demosaic_12in16.tif";
 	string opath = ipath + "results.tif";
 	string oUYVYhalf = ipath + "UYVYhalf.txt";
 	CuviBayerSeq sensorAlignment = CuviBayerSeq::CUVI_BAYER_RGGB;
 	Cuvi32s imageWidth = 1936;
 	Cuvi32s imageHeight = 1216;
 	Cuvi32s containerBits = 16; // In case of unsigned char data, change to 8
 	Cuvi32s dataBits = 12;     // 12 bits in 16 container, this setting this is very important. Change to 8 for 8-bit data.



 	/*************************Internal Parameters*************************/

 	Cuvi32s bytesPerPixel = containerBits / 8;
 	CuviSize imageSize(imageWidth, imageHeight);
 	Cuvi32s CPUinputPitch = bytesPerPixel * imageWidth; // edit this only if CPU pitch is different than width of image in bytes
 	Cuvi32s CPUoutputPitch = imageWidth * 2;
 	CuviStatus st;


 	// Note CUVI makes its own pitch which might be different from input's width step
 	CuviImage input(imageSize, containerBits, 1);
 	CuviImage buffer(imageSize, containerBits, 3); // for saving 12 bit RGB image
 	CuviImage bufferFullBright(imageSize, 8, 3); // for saving 8 bit RGB image
 	CuviImage buffer2(imageWidth / 2, imageHeight / 2, CuviType::CUVI_8UC3); //8 bit RGB half size
 	CuviImage outputHalf(imageWidth / 2, imageHeight / 2, CuviType::CUVI_8UC2); // UYVY image in half size


 	//Input data pointer
 	unsigned short *CFA;
 	size_t size = imageWidth * imageHeight * sizeof(unsigned short);
 	CFA = (unsigned short *)malloc(size);

 	/* Test Image Data filled into a pointer */
 	CuviImage image(ipath, CUVI_LOAD_IMAGE_GRAYSCALE_KEEP_DEPTH);
 	image.download(CFA, CPUinputPitch);
 	/* End Test filling of Data */


 	// For saving Output
 	unsigned char *UYVYhalf;
 	size_t sizeUYVYhalf = imageWidth * imageHeight * 0.5 * sizeof(unsigned char);
 	UYVYhalf = (unsigned char *)malloc(sizeUYVYhalf);
 	memset(UYVYhalf, 2, sizeUYVYhalf);


 	//Upload image to GPU from host pointer
 	st = input.upload(CFA, CPUinputPitch); if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }

 	//Perform Demosaic DFPD
 	buffer.setDataBits(dataBits); // Algo with ceil the output data to dataBits
 	st = cuvi::colorOperations::demosaicDFPD(input, buffer, sensorAlignment); if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }

 	//Convert 12 bit image to (8 bit image in an 8 bit container). 
 	//The function automatically converts the dataBits of input to comtainer size of output
 	st = cuvi::dataExchange::bitConversion(buffer, bufferFullBright); if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }
	
 	// save Output 1 (8-bitRGB image)
 	st = cuvi::io::saveImage(bufferFullBright, opath); if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }


 	//Resize image to half and perform RGB to UYVY
 	st = cuvi::geometryTransforms::resize(bufferFullBright, buffer2); /*resize*/  if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }
 	st = cuvi::colorOperations::rgb2yuv422(buffer2, outputHalf); /* UYVY half*/  if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }


 	//Download UYVY output Image to CPU
 	st = outputHalf.download(UYVYhalf, (CPUoutputPitch *0.5));  if (CUVI_SUCCESS != st) { cout << "CUVI Error: " << st << endl;	return; }


 	//Saving output data (half sized) into file
 	std::ofstream outfileHalf(oUYVYhalf, std::ofstream::binary);
 	outfileHalf.write((char *)&UYVYhalf[0], sizeUYVYhalf);
 	outfileHalf.close();


    cout << "\nProgram 3 executed successfully.." << endl;
    return;
	


 }







int main()
{

	Cuvi32s device = cuvi::device::getCurentDevice();
	if (device < 0) {
		cout << "Error: None or Unsupported CUDA Device Found" << endl;
		system("pause");
		return 0;
	}

	demosaic1();
    // demosaic2();
	// demosaic3();


	return 0;

	
	
}