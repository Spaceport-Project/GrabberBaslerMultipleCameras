from pypylon import genicam
from pypylon import pylon
import platform
import time

# Image event handler.
image_num1 = 1
image_num2 = 1
image_num3 = 1
image_num4 = 1
class SampleImageEventHandler1(pylon.ImageEventHandler):
    def OnImageGrabbed(self, camera, grabResult):
        img = pylon.PylonImage()
        img.AttachGrabResultBuffer(grabResult)
        global image_num1
        if platform.system() == 'Windows':
            # The JPEG format that is used here supports adjusting the image
            # quality (100 -> best quality, 0 -> poor quality).
            ipo = pylon.ImagePersistenceOptions()
            quality = 90
            ipo.SetQuality(quality)

            filename = "F:/Image/test/saved_pypylon_img_1_%d.jpeg" % image_num1
            img.Save(pylon.ImageFileFormat_Jpeg, filename, ipo)
            image_num1 = image_num1 + 1
        else:
            filename = "F:/Image/test/saved_pypylon_img_1_%d.png" % image_num1
            img.Save(pylon.ImageFileFormat_Png, filename)
            image_num1 = image_num1 + 1

        # In order to make it possible to reuse the grab result for grabbing
        # again, we have to release the image (effectively emptying the
        # image object).
        img.Release()

class SampleImageEventHandler2(pylon.ImageEventHandler):
    def OnImageGrabbed(self, camera, grabResult):
        img = pylon.PylonImage()
        img.AttachGrabResultBuffer(grabResult)
        global image_num2
        if platform.system() == 'Windows':
            # The JPEG format that is used here supports adjusting the image
            # quality (100 -> best quality, 0 -> poor quality).
            ipo = pylon.ImagePersistenceOptions()
            quality = 90
            ipo.SetQuality(quality)

            filename = "F:/Image/test/saved_pypylon_img_2_%d.jpeg" % image_num2
            img.Save(pylon.ImageFileFormat_Jpeg, filename, ipo)
            image_num2 = image_num2 + 1
        else:
            filename = "F:/Image/test/saved_pypylon_img_2_%d.png" % image_num2
            img.Save(pylon.ImageFileFormat_Png, filename)
            image_num2 = image_num2 + 1

        # In order to make it possible to reuse the grab result for grabbing
        # again, we have to release the image (effectively emptying the
        # image object).
        img.Release()



class SampleImageEventHandler3(pylon.ImageEventHandler):
    def OnImageGrabbed(self, camera, grabResult):
        img = pylon.PylonImage()
        img.AttachGrabResultBuffer(grabResult)
        global image_num3
        if platform.system() == 'Windows':
            # The JPEG format that is used here supports adjusting the image
            # quality (100 -> best quality, 0 -> poor quality).
            ipo = pylon.ImagePersistenceOptions()
            quality = 90
            ipo.SetQuality(quality)

            filename = "F:/Image/test/saved_pypylon_img_3_%d.jpeg" % image_num3
            img.Save(pylon.ImageFileFormat_Jpeg, filename, ipo)
            image_num3 = image_num3 + 1
        else:
            filename = "F:/Image/test/saved_pypylon_img_3_%d.png" % image_num3
            img.Save(pylon.ImageFileFormat_Png, filename)
            image_num3 = image_num3 + 1

        # In order to make it possible to reuse the grab result for grabbing
        # again, we have to release the image (effectively emptying the
        # image object).
        img.Release()

class SampleImageEventHandler4(pylon.ImageEventHandler):
    def OnImageGrabbed(self, camera, grabResult):
        img = pylon.PylonImage()
        img.AttachGrabResultBuffer(grabResult)
        global image_num4
        if platform.system() == 'Windows':
            # The JPEG format that is used here supports adjusting the image
            # quality (100 -> best quality, 0 -> poor quality).
            ipo = pylon.ImagePersistenceOptions()
            quality = 90
            ipo.SetQuality(quality)

            filename = "F:/Image/test/saved_pypylon_img_4_%d.jpeg" % image_num4
            img.Save(pylon.ImageFileFormat_Jpeg, filename, ipo)
            image_num4 = image_num4 + 1
        else:
            filename = "F:/Image/test/saved_pypylon_img_4_%d.png" % image_num4
            img.Save(pylon.ImageFileFormat_Png, filename)
            image_num4 = image_num4 + 1

        # In order to make it possible to reuse the grab result for grabbing
        # again, we have to release the image (effectively emptying the
        # image object).
        img.Release()




if __name__ == '__main__':
    try:
        # Get the transport layer factory.
        tlFactory = pylon.TlFactory.GetInstance()
        # Get all attached devices and exit application if no device is found.
        devices = tlFactory.EnumerateDevices()
        if len(devices) == 0:
            raise pylon.RuntimeException("No cameta present.")
        # Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
        cameras = pylon.InstantCameraArray(min(len(devices), 4))

        for i, cam in enumerate(cameras):
            # Attach all Pylon Devices.
            cam.Attach(tlFactory.CreateDevice(devices[i]))
            cam.Open()
            # Enable PTP
            # cam.GevIEEE1588.SetValue(True)
            cam.PtpEnable.SetValue(True)
            # Make sure the frame trigger is set to Off to enable free run
            cam.TriggerSelector.SetValue('FrameStart')
            cam.TriggerMode.SetValue('Off')
            # Let the free run start immediately without a specific start time
            cam.SyncFreeRunTimerStartTimeLow.SetValue(0)
            cam.SyncFreeRunTimerStartTimeHigh.SetValue(0)
            # Set the trigger rate to 10 frames per second
            cam.SyncFreeRunTimerTriggerRateAbs.SetValue(10)
            # Apply the changes
            cam.SyncFreeRunTimerUpdate.Execute()
            # Start the synchronous free run
            cam.SyncFreeRunTimerEnable.SetValue(True)
            
        # create and register an image event handler processing the grab results
        cameras[0].RegisterImageEventHandler(SampleImageEventHandler1(), pylon.RegistrationMode_Append, pylon.Cleanup_Delete)
        cameras[1].RegisterImageEventHandler(SampleImageEventHandler2(), pylon.RegistrationMode_Append, pylon.Cleanup_Delete)
        cameras[2].RegisterImageEventHandler(SampleImageEventHandler3(), pylon.RegistrationMode_Append, pylon.Cleanup_Delete)
        cameras[3].RegisterImageEventHandler(SampleImageEventHandler4(), pylon.RegistrationMode_Append, pylon.Cleanup_Delete)
        # Start the grabbing using the grab loop thread, by setting the grabLoopType parameter
        # to GrabLoop_ProvidedByInstantCamera. The grab results are delivered to the image event handlers.
        # The GrabStrategy_OneByOne default grab strategy is used.
        cameras[0].StartGrabbing(pylon.GrabStrategy_OneByOne, pylon.GrabLoop_ProvidedByInstantCamera)
        cameras[1].StartGrabbing(pylon.GrabStrategy_OneByOne, pylon.GrabLoop_ProvidedByInstantCamera)
        cameras[2].StartGrabbing(pylon.GrabStrategy_OneByOne, pylon.GrabLoop_ProvidedByInstantCamera)
        cameras[3].StartGrabbing(pylon.GrabStrategy_OneByOne, pylon.GrabLoop_ProvidedByInstantCamera)
        while True:
            time.sleep(0.05)
        

    except genicam.GenericException as e:
        # Error handling.
        print("An exception occurred.", e.GetDescription())