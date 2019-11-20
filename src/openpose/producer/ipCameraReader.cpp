#include <openpose/producer/ipCameraReader.hpp>
#include <openpose/utilities/fastMath.hpp>
#include <openpose/utilities/openCv.hpp>
#include <openpose_private/utilities/openCvMultiversionHeaders.hpp>
#include <openpose_private/utilities/openCvPrivate.hpp>

namespace op
{
    // Public IP cameras for testing (add ?x.mjpeg):
    // http://iris.not.iac.es/axis-cgi/mjpg/video.cgi?resolution=320x240?x.mjpeg
    // http://www.webcamxp.com/publicipcams.aspx

    IpCameraReader::IpCameraReader(const std::string & cameraPath, const std::string& cameraParameterPath,
                                   const bool undistortImage) :
        VideoCaptureReader{cameraPath, ProducerType::IPCamera, cameraParameterPath, undistortImage, 1},
        mPathName{cameraPath},
    	mThreadOpened{std::atomic<bool>{false}}
    {
	try
        {
	    if (isOpened())
	    {
		// Start buffering thread
                mThreadOpened = true;
                mThread = std::thread{&IpCameraReader::bufferingThread, this};
	    }
	}
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    IpCameraReader::~IpCameraReader()
    {
	try
        {
            // Close and join thread
            if (mThreadOpened)
            {
                mCloseThread = true;
                mThread.join();
            }
        }
        catch (const std::exception& e)
        {
            errorDestructor(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    std::string IpCameraReader::getNextFrameName()
    {
        try
        {
            return VideoCaptureReader::getNextFrameName();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return "";
        }
    }

    Matrix IpCameraReader::getRawFrame()
    {
        try
        {
	    // Retrieve frame from buffer
            Matrix opMat;
            auto cvMatRetrieved = false;
            while (!cvMatRetrieved)
            {
                // Retrieve frame
                std::unique_lock<std::mutex> lock{mBufferMutex};
                if (!mBuffer.empty())
                {
                    std::swap(opMat, mBuffer);
                    cvMatRetrieved = true;
                }
                // No frames available -> sleep & wait
                else
                {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::microseconds{5});
                }
            }
            return opMat;
	    //return VideoCaptureReader::getRawFrame();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return Matrix();
        }
    }

    std::vector<Matrix> IpCameraReader::getRawFrames()
    {
        try
        {
            return VideoCaptureReader::getRawFrames();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return {};
        }
    }

    void IpCameraReader::bufferingThread()
    {
        try
        {
            mCloseThread = false;
            while (!mCloseThread)
            {
		// Get frame
                auto opMat = VideoCaptureReader::getRawFrame();

		if (!opMat.empty())
                {
                    const std::lock_guard<std::mutex> lock{mBufferMutex};
                    std::swap(mBuffer, opMat);
                }
	    }
	}
	catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
}
