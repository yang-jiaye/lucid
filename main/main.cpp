//xin
/*#include <cstring>
#include <string>
#include <vector>
#include "GenApi/GenApi.h"
//#include "GenApiCustom.h"*/


#include "stdafx.h"
#include "ArenaApi.h"
#include <algorithm> // for std::find
#include <thread>    // for sleep
#include "SaveApi.h" // for Save

#define TAB1 "  "
#define TAB2 "    "
#define TAB3 "      "
#define ERASE_LINE "                            "

// Scheduled Action Commands
//     This exmample introduces scheduling action commands on multiple cameras.
//     The device settings are configured to allow each device to trigger a
//     single image using action commands. The system is prepared to receive an
//     action command and the devices's PTP relationships are synchronized This
//     allowsactions commands to be fired across all devices, resulting in
//     simultaneously acquired images with synchronized timestampes. Depending
//     on the initial PTP state of each camera, it can took about 40 seconds for
//     all devices wo autonegotiate.

// =-=-=-=-=-=-=-=-=-
// =-=- SETTINGS =-=-
// =-=-=-=-=-=-=-=-=-


// delta time (ns)
#define DELTA_TIME 1000000000

// exposure time
//#define EXPOSURE_TIME 500.0 //in milliseconds
#define EXPOSURE_TIME 21661.5 //in milliseconds//xin 2023-9-4 : from Window's ArenaView


// File name pattern
//    File name patterns can use tags to easily customize your file names.
//    Customizable tags can be added to a file name pattern and later set on the
//    fly. Two tags, <count> and <datetime> have been built in to the save
//    library. As seen below, <datetime> can take an argument to specify output.
//    <count> also accepts arguments (local, path, and global) to specify what
// exactly is being counted.
//#define FILE_NAME_PATTERN "Images/Cpp_Save_FileNamePattern/<vendor>_<model>_<serial>_image<count>-<datetime:yyMMdd_hhmmss_fff>.bmp"
#define FILE_NAME_PATTERN "data/<vendor>_<model>_<serial>_image<count>-<datetime:yyMMdd_hhmmss_fff>.PNG"


// pixel format//xin: Cpp_Save_FileNamePattern
//#define PIXEL_FORMAT RGB24
//#define PIXEL_FORMAT BayerRG8
#define PIXEL_FORMAT BGR8


// =-=-=-=-=-=-=-=-=-
// =-=- EXAMPLE -=-=-
// =-=-=-=-=-=-=-=-=-

// demonstrates action commands
// (1) manually sets exposure, trigger and action command settings
// (2) prepares devices for action commands
// (3) synchronizes devices and fire action command
// (4) retrieves images with synchronized timestamps
void SynchronizeCamerasAndTriggerImage(Arena::ISystem* pSystem, std::vector<Arena::IDevice*>& devices)
{
	// get node values that will be changed in order to return their values at
	// the end of the example 
	std::vector<GenICam::gcstring> exposureAutoInitials;//TODO: not present in the app note
	std::vector<double>            exposureTimeInitials;//TODO: not present in the app note
	std::vector<bool>              ptpEnableInitials;
	std::vector<GenICam::gcstring> triggerModeInitials;
	std::vector<GenICam::gcstring> triggerSourceInitials;
	std::vector<GenICam::gcstring> triggerSelectorInitials;
	std::vector<GenICam::gcstring> actionUnconditionalModeInitials;
	std::vector<int64_t>           actionSelectorInitials;
	std::vector<int64_t>           actionGroupKeyInitials;
	std::vector<int64_t>           actionGroupMaskInitials;
	std::vector<GenICam::gcstring> transferControlModeInitials;
	std::vector<int64_t>           packetSizeInitials;//TODO: not present in the app note
	// TODO: missing 'TransferOperationMode', 'ActionDeviceKey'
	for (size_t i = 0; i <devices.size(); i++)
	{
		Arena::IDevice* pDevice = devices.at(i);
		// exposure
		exposureAutoInitials.push_back(Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ExposureAuto"));
		exposureTimeInitials.push_back(Arena::GetNodeValue<double>(pDevice->GetNodeMap(), "ExposureTime"));
		// trigger
		triggerModeInitials.push_back(Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerMode"));
		triggerSourceInitials.push_back(Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerSource"));
		triggerSelectorInitials.push_back(Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerSelector"));
		// action commands//TODO: missing ActionDeviceKey
		actionUnconditionalModeInitials.push_back(Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ActionUnconditionalMode"));
		actionSelectorInitials.push_back(Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "ActionSelector"));
		actionGroupKeyInitials.push_back(Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "ActionGroupKey"));
		actionGroupMaskInitials.push_back(Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "ActionGroupMask"));
		// ptp
		ptpEnableInitials.push_back(Arena::GetNodeValue<bool>(pDevice->GetNodeMap(), "PtpEnable"));
		// Transfer control//TODO: missing TransferOperationMode
		transferControlModeInitials.push_back(Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TransferControlMode"));
		// packet size
		packetSizeInitials.push_back(Arena::GetNodeValue<int64_t>(pDevice->GetNodeMap(), "DeviceStreamChannelPacketSize"));
	}

	// prepare all cameras
	std::cout << TAB1 << "Setup\n";

	std::vector<Save::ImageWriter> writers;
	for (size_t i = 0; i <devices.size(); i++)
	{
		Arena::IDevice* pDevice = devices.at(i);
		GenICam::gcstring deviceSerialNumber = Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "DeviceSerialNumber");

		std::cout << TAB2 << "Prepare camera " << deviceSerialNumber << "\n";

		// Manually set exposure time
		//    In order to get synchronized images, the exposure time must be
		//    synchronized as well.
		std::cout << TAB3 << "Exposure: ";

		Arena::SetNodeValue<GenICam::gcstring>(
				pDevice->GetNodeMap(),
				"ExposureAuto",
				"Off");

		Arena::SetNodeValue<double>(
				pDevice->GetNodeMap(),
				"ExposureTime",
				EXPOSURE_TIME);

		std::cout << Arena::GetNodeValue<double>(pDevice->GetNodeMap(), "ExposureTime") << '\n';

		// Enable trigger mode and set source to action
		//    To trigger a single image using action commands, trigger mode must
		//    be enabled, the source set to an action command, and the selector
		//    set to the start of a frame.
		std::cout << TAB3 << "Trigger: ";

		Arena::SetNodeValue<GenICam::gcstring>(
				pDevice->GetNodeMap(),
				"TriggerSelector",
				"FrameStart");

		Arena::SetNodeValue<GenICam::gcstring>(
				pDevice->GetNodeMap(),
				"TriggerMode",
				"On");

		Arena::SetNodeValue<GenICam::gcstring>(
				pDevice->GetNodeMap(),
				"TriggerSource",
				"Action0");

		std::cout << Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerSource") << "\n";

	    // Prepare the device to receive an action command
		//    Action unconditional mode allows a camera to accept action from an
		//    application without write access. The device key, group key, and
		//    group mask must match similar settings in the system's TL node map.//TODO: what is a "system's TL node"?
		std::cout << TAB3 << "Action commands: ";

		Arena::SetNodeValue<GenICam::gcstring>(
				pDevice->GetNodeMap(),
				"ActionUnconditionalMode",
				"On");

		Arena::SetNodeValue<int64_t>(
				pDevice->GetNodeMap(),
				"ActionSelector",
				0);

		Arena::SetNodeValue<int64_t>(
				pDevice->GetNodeMap(),
				"ActionDeviceKey",
				1);//TODO: set as 'g_action_device_key' in the app note

		Arena::SetNodeValue<int64_t>(
				pDevice->GetNodeMap(),
				"ActionGroupKey",
				1);//TODO: set as 'g_action_group_key' in the app note

		Arena::SetNodeValue<int64_t>(
				pDevice->GetNodeMap(),
				"ActionGroupMask",
				1);//TODO: set as 'g_action_group_mask' in the app note

		std::cout << "prepared\n";

		// Synchronize devices by enabling PTP
		//    Enabling PTP on multiple devices causes them to negotiate amongst
		//    themselves to that there is a single master device while all the
		//    rest becomes slaves. The slaves' clocks all synchronize to the
		//    master's clock.
		std::cout << TAB3 << "PTP: ";

		Arena::SetNodeValue<bool>(
				pDevice->GetNodeMap(),
				"PtpEnable",
				true);

		std::cout << (Arena::GetNodeValue<bool>(pDevice->GetNodeMap(), "PtpEnable") ? "enabled" : "disabled") << '\n';

		// Use max supported packet size. We use transfer control to ensure that
		// only one camera is transmitting at a time.
		Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);

		// enable stream packet resend
		Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);

		// Enable user controlled transfer control
		//    Synchronized cameras will begin transmitting images at the same
		//    time. To avoid missing packets due to collision, we will use
		//    transfer control to control when each camera transmits the image.
		std::cout << TAB3 << "Transfer Control: ";

		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TransferControlMode", "UserControlled");
		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TransferOperationMode", "Continuous");
		Arena::ExecuteNode(pDevice->GetNodeMap(), "TransferStop");

		std::cout << Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TransferControlMode") << " - " << Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TransferOperationMode") << " - " << "Transfer Stopped\n";


		//xin Cpp_Save_FileNamePattern
		// get width, height, and pixel format nodes
		GenApi::CIntegerPtr pWidth = pDevice->GetNodeMap()->GetNode("Width");
		GenApi::CIntegerPtr pHeight = pDevice->GetNodeMap()->GetNode("Height");
		GenApi::CEnumerationPtr pPixelFormat = pDevice->GetNodeMap()->GetNode("PixelFormat");

		if (!pWidth || !pHeight || !pPixelFormat)
		{
			throw GenICam::GenericException("Width, Height, or PixelFormat node could not be found", __FILE__, __LINE__);
		}

		if (!GenApi::IsReadable(pWidth) || !GenApi::IsReadable(pHeight) || !GenApi::IsReadable(pPixelFormat))
		{
			throw GenICam::GenericException("Width, Height, or PixelFormat node not readable", __FILE__, __LINE__);
		}

		// Prepare image parameters from device settings
		//    An image's width, height, and bits per pixel are required to save to
		//    disk. Its size and stride (i.e. pitch) can be calculated from those 3
		//    inputs. Notice that an image's size and stride use bytes as a unit
		//    while the bits per pixel uses bits. Arena defines its pixel formats by
		//    the PFNC (Pixel Format Naming Convention), which embeds the number of
		//    bits per pixel within a pixel format's integer representation.
		std::cout << TAB1 << "Prepare image parameters\n";

		Save::ImageParams params(
			static_cast<size_t>(pWidth->GetValue()),
			static_cast<size_t>(pHeight->GetValue()),
			Arena::GetBitsPerPixel(pPixelFormat->GetCurrentEntry()->GetValue()));

		// Prepare image writer
		//    The image writer requires 3 arguments to save an image: the image's
		//    parameters, a specified file name or pattern, and the image data to
		//    save. If a file name pattern uses the <timestamp> tag, then a timestamp
		//    must also be provided. Providing these should result in a successfully
		//    saved file on the disk. Because an image's parameters and file name
		//    pattern may repeat, they can be passed into the image writer's
		//    constructor. However, they can also be passed in dynamically using the
		//    cascading I/O operator (<<).
		std::cout << TAB1 << "Prepare image writer\n";

		Save::ImageWriter writer(
			params,
			FILE_NAME_PATTERN);

		// Update tags
		//    Tags are set on the fly by passing strings into the cascading I/O
		//    operator. Tags are accepted as any string surrounded by angle brackets
		//    <...> while values are accepted as everything else. A value will be set
		// to the last input tag.
		std::cout << TAB1 << "Update tags\n";

		// <vendor> tag
		std::cout << TAB2 << "<vendor> to LUCID\n";

		// get <model> tag
		GenICam::gcstring model = Arena::GetNodeValue<GenICam::gcstring>(
			pDevice->GetNodeMap(),
			"DeviceModelName");

		std::cout << TAB2 << "<model> to " << model << "\n";

		// get <serial> tag
		GenICam::gcstring serial = Arena::GetNodeValue<GenICam::gcstring>(
			pDevice->GetNodeMap(),
			"DeviceSerialNumber");

		std::cout << TAB2 << "<serial> to " << serial << "\n";

		// update
		writer << "<vendor>"
			<< "LUCID"

			<< "<model>"
			<< model

			<< "<serial>"
			<< serial

			<< "<count>"
			<< i;
		writers.push_back(writer);
	}

	// prepare system
	std::cout << TAB2 << "Prepare system\n";
	
	// Prepare the system to broadcast an action command
	//    The device key, group key, group mask, and target IP must all match
	//    similar settings in the devices' node maps. The target IP acts as a
	//    mask.
	std::cout << TAB3 << "Action commands: ";

	Arena::SetNodeValue<int64_t>(
			pSystem->GetTLSystemNodeMap(),
			"ActionCommandDeviceKey",
			1);

	Arena::SetNodeValue<int64_t>(
			pSystem->GetTLSystemNodeMap(),
			"ActionCommandGroupKey",
			1);

	Arena::SetNodeValue<int64_t>(
			pSystem->GetTLSystemNodeMap(),
			"ActionCommandGroupMask",
			1);

	Arena::SetNodeValue<int64_t>(
			pSystem->GetTLSystemNodeMap(),
			"ActionCommandTargetIP",
			0xFFFFFFFF);

	std::cout << "prepared\n";

	// Wait for devices to negotiate their PTP relationship
	//    Before starting any PTP-dependent actions, it is important to wait for
	//    the devices to complete their negotiation; otherwise, the devices may
	//    not yet be synced. Depending on the initial PTP state of each camera,
	//    it can take 40 seconds for all devices to autonegotiate. Below, we
	//    wait for the PTP status of each device until there is only one 
	//    'Master' and the rest are all 'Slaves'. During the negotiation phase,
	//    multiple devices may initially come up as Master so we will wait until
	//    the ptp negotiation completes.
	std::cout << TAB1 << "Wait for devices to negotiate. This can take up to 40s. \n";

	std::vector<GenICam::gcstring> serials;
	int i = 0;
	do
	{
		bool masterFound = false;
		bool restartSyncCheck = false;

		// check device
		for (size_t j = 0; j < devices.size(); j++)
		{
			Arena::IDevice* pDevice = devices.at(j);

			// get PTP status
			GenICam::gcstring ptpStatus = Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "PtpStatus");

//	Arena::SetNodeValue(pDevice->GetNodeMap(),"AcquisitionStartMode", "PTPSync"); //xin 20230902: not supported yet. see
//	support.thinklucid.com/app-note-bandwidth-sharing-in-multi-camera-systems
			if (ptpStatus == "Master")
			{
				if (masterFound)
				{
					// Multiple masters -- ptp negotiation is not complete
					restartSyncCheck = true;
					break;
				}

				masterFound = true;
			}
			else if (ptpStatus != "Slave")
			{
				// Uncalibrated state -- ptp negotiation is not complete
				restartSyncCheck = true;
				break;
			}
		}

		// A single master was found and all remaining cameras are slaves
		if (!restartSyncCheck && masterFound)
			break;
		std::this_thread::sleep_for(std::chrono::duration<int>(1));

		// for output
		if (i % 10 == 0)
			std::cout << "\r" << ERASE_LINE << "\r" << TAB2 << std::flush;

		std::cout << "." <<std::flush;

		i++;

	} while (true);

	// start stream
	std::cout << "\n" << TAB1 << "Start stream\n";

	for (size_t i = 0; i < devices.size(); i++)
	{
		devices.at(i)->StartStream();
	}

	for (size_t i = 0; i < 600; i++) // xin: num of images to take
	{
		std::cout<<std::endl<<"i = "<<i<<std::endl;
		// Set up timing and broadcast action command
		//    Action commands must be scheduled for a time in the future. This can be
		//    done by grabbing the PTP time from a device, adding a delta to it. and
		//    setting it as an action command's execution time.
		std::cout << TAB1 << "Set action command to " << DELTA_TIME << " nanoseconds from now\n";

		// execute latch
		Arena::ExecuteNode(
				devices.at(0)->GetNodeMap(),
				"PtpDataSetLatch");

		// get latch
		int64_t ptpDataSetLatchValue = Arena::GetNodeValue<int64_t>(
				devices.at(0)->GetNodeMap(),
				"PtpDataSetLatchValue");

		// set execute time to future time
		Arena::SetNodeValue<int64_t>(
				pSystem->GetTLSystemNodeMap(),
				"ActionCommandExecuteTime",
				ptpDataSetLatchValue + DELTA_TIME);

		// Fire action command
		//    Action commands are fired and broadcast to all devices, but only
		//    received by the devices matching desired settings.
		std::cout << TAB1 << "Fire action command\n";

		Arena::ExecuteNode(
				pSystem->GetTLSystemNodeMap(),
				"ActionCommandFireCommand");

		// get images and check timestamps
		std::cout << TAB1 << "Get images\n";
		for (size_t i = 0; i < devices.size(); i++)
		{
			Arena::IDevice* pDevice = devices.at(i);
			GenICam::gcstring deviceSerialNumber = Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "DeviceSerialNumber");

			std::cout << TAB2 << "Image from device " << deviceSerialNumber << "\n";

			// Compare timestampes
			//    Scheduling action commands amongst PTP synchronized devices
			//    results in synchronized images with synchronized timestamps.
			std::cout << TAB3 << "Timestamp: ";

			// Initiate image transfer from current camera
			Arena::ExecuteNode(pDevice->GetNodeMap(), "TransferStart");

			Arena::IImage* pImage = pDevice->GetImage(3000);

			Arena::ExecuteNode(pDevice->GetNodeMap(), "TransferStop");

			std::cout << pImage->GetTimestamp() << "\n";
			
			// xin: Cpp_Save_FileNamePattern
			// convert the image to a displayable pixel format.
			Arena::IImage* pConverted = Arena::ImageFactory::Convert(
					pImage,
					PIXEL_FORMAT);
			// save image
			writers.at(i) << pConverted->GetData();

			// Get last file name
			//    The image writer allows for the retrieval of paths, file names, and
			//    extensions. They can be retrieved together or separately, and it is
			//    possible to get the pattern, peek ahead at the next file name, or
			//    get the last file name.
			std::cout << " at " << writers.at(i).GetLastFileName(true) << "\n";

			// destroy converted image
			Arena::ImageFactory::Destroy(pConverted);



			// requeue buffer(: xin) in order to resuse this buffer a GenTL
			// Consumer has to pt the buffer back into the Input Buffer Pool (requeue)
			// emva.org/wp-content/uploads/GenICam_GenTL_1_5.pdf (page 42 of 161)
			pDevice->RequeueBuffer(pImage);
		}
	}
	
	
	std::cout << "Xin's Lucid program ended.\n";

	// stop stream
	for (size_t i = 0; i < devices.size(); i++)
	{
		devices.at(i)->StopStream();
	}
	std::cout << "\n" << TAB1 <<"StopStream completed\n";

	// return nodes to their initial values
	for (size_t i = 0; i < devices.size(); i++)
	{
		Arena::IDevice* pDevice = devices.at(i);

		// packet size affects the exposure range so we restore it first
		//xin-2023-4-14//Arena::SetNodeValue<int64_t>(pDevice->GetNodeMap(), "DeviceStreamChannelPacketSize", packetSizeInitials.at(i));

		// exposure
		if (exposureAutoInitials.at(i) == "OFF")
		{
			Arena::SetNodeValue<double>(pDevice->GetNodeMap(), "ExposureTime", exposureTimeInitials.at(i));
		}
		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ExposureAuto", exposureAutoInitials.at(i));
		// trigger
		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerSelector", triggerSelectorInitials.at(i));
		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerSource", triggerSourceInitials.at(i));
		//xin-2023-4-14//Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TriggerMode", triggerModeInitials.at(i));
		// action commands
		Arena::SetNodeValue<int64_t>(pDevice->GetNodeMap(), "ActionGroupMask", actionGroupMaskInitials.at(i));
		Arena::SetNodeValue<int64_t>(pDevice->GetNodeMap(), "ActionGroupKey", actionGroupKeyInitials.at(i));
		Arena::SetNodeValue<int64_t>(pDevice->GetNodeMap(), "ActionSelector", actionSelectorInitials.at(i));
		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ActionUnconditionalMode", actionUnconditionalModeInitials.at(i));
		// ptp
		Arena::SetNodeValue<bool>(pDevice->GetNodeMap(), "PtpEnable", ptpEnableInitials.at(i));

		// Transfer Control
		Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "TransferControlMode", transferControlModeInitials.at(i));
	}
	std::cout << "\n" << TAB1 << "Nodes returned to their initial values.\n";
	return;
}

// =-=-=-=-=-=-=-=-=-
// =- PREPARATION -=-
// =- & CLEAN UP =-=-
// =-=-=-=-=-=-=-=-=-

int main()
{
	//flag to track when an exception has been thrown
	bool exceptionThrown = false;

	std::cout << "Cpp_ScheduledActionCommands\n";
	std::cout << "\nNote: The PTP auto-negotiation phase can take about 40s depending on the "
		<< "initial PTP state of each device\n\n";
	std::cout << "Example may overwrite 'ActionDeviceKey' -- proceed? ('y' to continue) ";
	char continueExample = 'a';
	std::cin >> continueExample;

	if (continueExample == 'y')
	{
		try
		{
			// prepare example
			Arena::ISystem* pSystem = Arena::OpenSystem();
			pSystem->UpdateDevices(100);
			std::vector<Arena::DeviceInfo> deviceInfos = pSystem->GetDevices();
			if (deviceInfos.size() < 2)
			{
				if (deviceInfos.size() == 0)
					std::cout << "\nNo camera connected. Example requires at least 2 devices.\n";
				else if (deviceInfos.size() == 1)
					std::cout << "\nOnly one device connected. Example requires at least 2 devices.\n";
				
				std::cout << "Press enter to complete\n";

				// clear input
				while (std::cin.get() != '\n')
					continue;

				std::getchar();
				return 0;
			}
			std::vector<Arena::IDevice*> devices;
			for (size_t i = 0; i < deviceInfos.size(); i++)
			{
				devices.push_back(pSystem->CreateDevice(deviceInfos.at(i)));
			}

			// run example
			std::cout << "\nCommence example\n\n";
			SynchronizeCamerasAndTriggerImage(pSystem, devices);
			std::cout << "\nExample complete\n";

			// clean up example
			for (size_t i = 0; i < devices.size(); i++)
			{
				pSystem->DestroyDevice(devices.at(i));
			}
			Arena::CloseSystem(pSystem);
		}
		catch (GenICam::GenericException& ge)
		{
			std::cout << "\nGenICam exception thrown:\n " << ge.what() << "\n";
			exceptionThrown = true;
		}
		catch (std::exception& ex)
		{
			std::cout << "\nStandard exception thrown: " << ex.what() << "\n";
			exceptionThrown = true;
		}
		catch (...)
		{
			std::cout << "\nUnexpected exception thrown\n";
			exceptionThrown = true;
		}
	}

	std::cout << "Press enter to complete\n";

	// clear input
	while (std::cin.get() != '\n')
		continue;

	std::getchar();

	if (exceptionThrown)
		return -1;
	else
		return 0;
}
