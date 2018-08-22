#include <data_tools/xtf_data.h>
extern "C" {
#include <libxtf/xtf_reader.h>
}
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
//#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
//#include <stdlib.h>
//#include <string.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <data_tools/lat_long_utm.h>

using namespace std;

cv::Mat make_waterfall_image(const xtf_sss_ping::PingsT& pings)
{
    int rows = pings.size();
    int cols = pings[0].port.pings.size() + pings[0].stbd.pings.size();
    cv::Mat swath_img = cv::Mat(rows, cols, CV_8UC3, cv::Scalar(255, 255, 255));
    for (int i = 0; i < pings.size(); ++i) {
        for (int j = 0; j < pings[i].port.pings.size(); ++j) {
            cv::Point3_<uchar>* p = swath_img.ptr<cv::Point3_<uchar> >(i, cols-j-1);
            p->z = uchar(255.*(float(pings[i].port.pings[j]) + 32767.)/(2.*32767.));
            p->y = uchar(255.*(float(pings[i].port.pings[j]) + 32767.)/(2.*32767.));
            p->x = uchar(255.*(float(pings[i].port.pings[j]) + 32767.)/(2.*32767.));
        }
        for (int j = 0; j < pings[i].stbd.pings.size(); ++j) {
            cv::Point3_<uchar>* p = swath_img.ptr<cv::Point3_<uchar> >(i, pings[0].stbd.pings.size()-j-1);
            //tie(p->z, p->y, p->x) = uchar(255.*(float(pings[i].port_pings[j]) + 32767.)/(2.*65536));
            p->z = uchar(255.*(float(pings[i].stbd.pings[j]) + 32767.)/(2.*32767.));
            p->y = uchar(255.*(float(pings[i].stbd.pings[j]) + 32767.)/(2.*32767.));
            p->x = uchar(255.*(float(pings[i].stbd.pings[j]) + 32767.)/(2.*32767.));
        }
    }
    cv::Mat resized_swath_img;//dst image
    cv::resize(swath_img, resized_swath_img, cv::Size(rows/8, cols/8));//resize image
    
    return resized_swath_img;
}

xtf_sss_ping process_side_scan_ping(XTFPINGHEADER *PingHeader, XTFFILEHEADER *XTFFileHeader) {
/****************************************************************************/
// Put whatever processing here to be performed on SIDESCAN data.
// PingHeader points to the 256-byte ping header structure.  That structure
// identifies how many channels of sidescan follow.  The structure is followed
// by the sidescan data itself.
//
// For example: assume there are two channels of sidescan, stored 1024
// 8-bit samples per channel.  The data pointed to by PingHeader looks like:
// 
// 256 bytes   - XTFPINGHEADER structure holds data about this ping
// 64 bytes    - XTFPINGCHANHEADER structure holds data about channel 1 (port)
// 1024 bytes  - channel 1 imagery
// 64 bytes    - XTFPINGCHANHEADER structure holds data about channel 2 (stbd)
// 1024 bytes  - channel 2 imagery
//
   const boost::posix_time::ptime epoch = boost::posix_time::time_from_string("1970-01-01 00:00:00.000");
   WORD chan;
   int tmp;
   unsigned char *Ptr = (unsigned char *)PingHeader;

   // For backwards-compatibility.  The samples per channel used to
   // be stored in the file header.  


   // skip past the ping header
   Ptr += sizeof(XTFPINGHEADER);

   xtf_sss_ping ping;
   ping.lat_ = PingHeader->SensorXcoordinate;
   ping.long_ = PingHeader->SensorYcoordinate;

   double easting, northing;
   string utm_zone;
   tie(northing, easting, utm_zone) = lat_long_to_UTM(ping.lat_, ping.long_);
   ping.pos_ = Eigen::Vector3d(easting, northing, -PingHeader->SensorDepth);
   ping.heading_ = M_PI/180.*PingHeader->SensorHeading;

   boost::posix_time::ptime data_time(boost::gregorian::date(PingHeader->Year, PingHeader->Month, PingHeader->Day), boost::posix_time::hours(PingHeader->Hour)+boost::posix_time::minutes(PingHeader->Minute)+boost::posix_time::seconds(PingHeader->Second)+boost::posix_time::milliseconds(10.*int(PingHeader->HSeconds))); 
   stringstream time_ss;
   time_ss << data_time;
   ping.time_string_ = time_ss.str();
   boost::posix_time::time_duration const diff = data_time - epoch;
   ping.time_stamp_ = diff.total_milliseconds();

   for (chan=0; chan<PingHeader->NumChansToFollow; chan++) {

      XTFPINGCHANHEADER *ChanHeader;
      short *Imagery; // since BytesPerSample is 2, should be signed 16 bit
      WORD ChannelNumber;
      WORD BytesPerSample;
      DWORD SamplesPerChan;
      DWORD BytesThisChannel;
      
      // point to the channel header
      //
      ChanHeader = (XTFPINGCHANHEADER *) Ptr;

      // See which channel this is (0=port, 1=stbd, etc...)
      //
      ChannelNumber = ChanHeader->ChannelNumber;

      // Compute the number of bytes of imagery for this channel
      //
      BytesPerSample   = XTFFileHeader->ChanInfo[ChannelNumber].BytesPerSample;

      // If the NumSamples value in the channel header is zero, then this
      // must be an old-style XTF File where the samples per channel was
      // stored in this file header.
      SamplesPerChan = XTFFileHeader->ChanInfo[ChannelNumber].Reserved; // for backwards compatibility only!
      tmp = ChanHeader->NumSamples;
      if (tmp != 0) SamplesPerChan = tmp;

      BytesThisChannel = BytesPerSample * SamplesPerChan;

      // skip past the channel header
      //
      Ptr += sizeof(XTFPINGCHANHEADER);

      // Point to the imagery.  If BytesPerSample is 2, then
      // Imagery should be a pointer to a signed 16-bit value.
      // If BytesPerSample is 1, then Imagery should point to 
      // a unsigned 8-bit value.
      Imagery = (short*)Ptr;
      xtf_sss_ping_side* ping_channel;
      if (ChannelNumber == 0) {
          ping_channel = &ping.port;
      }
      else if (ChannelNumber == 1) {
          ping_channel = &ping.stbd;
      }
      else {
          // skip past the imagery;
          Ptr += BytesThisChannel;
          continue;
      }
      for (int i = 0; i < SamplesPerChan; ++i) {
          // we should get port and starboard channel from header definition
          ping_channel->pings.push_back(Imagery[i]);
      }
      ping_channel->time_duration = ChanHeader->TimeDuration;
      ping_channel->slant_range = ChanHeader->SlantRange;

      // Do whatever processing on the sidescan imagery here.
      //cout << "Processing a side scan ping!!" << endl;
      cout << "Size of short: " << sizeof(short) << endl;
      cout << "Channel number: " << int(ChannelNumber) << endl;
      //cout << "Channel name: " << ChannelName << endl;
      cout << "Bytes per sample: " << int(BytesPerSample) << endl;
      cout << "Samples per chan: " << int(SamplesPerChan) << endl;
      cout << "Ground range: " << int(ChanHeader->GroundRange) << endl; // seems to always be 0
      cout << "Slant range: " << int(ChanHeader->SlantRange) << endl;
      cout << "Time duration: " << ChanHeader->TimeDuration << endl;
      cout << "SecondsPerPing: " << ChanHeader->SecondsPerPing << endl; // seems to always be 0

      // skip past the imagery;
      Ptr += BytesThisChannel;
   }

   return ping;
}

xtf_sss_ping::PingsT read_xtf_file(int infl, XTFFILEHEADER* XTFFileHeader, unsigned char* buffer)
{
    /***************************************************************************************/
    // Given a handle to on open .XTF file, read through the file and
    // print out some data about it.

    //
    // Read the XTF file header
    //

    xtf_sss_ping::PingsT pings;
    if (ReadXTFHeader(infl, XTFFileHeader, buffer) == FALSE) {
        return pings;
    }

    ProcessXTFHeader(infl, XTFFileHeader, buffer);

    //
    // Read the XTF file one packet at a time
    //

    unsigned int amt;
    char* ptr;
    while ((amt = ReadXTFFormatFileData(infl, buffer)) != 0xFFFF) {
        //
        // Buffer now holds a single record which can be processed
        // here.  The return value from ReadXTFFormatFileData()
        // holds byte length of the data record (i.e., sidescan ping)
        //

        XTFPINGHEADER* PingHeader = (XTFPINGHEADER*)buffer;

        if (PingHeader->HeaderType != XTF_HEADER_SONAR) {
            continue;
        }

        xtf_sss_ping ping = process_side_scan_ping((XTFPINGHEADER*)PingHeader, XTFFileHeader);
        ping.first_in_file_ = false;
        pings.push_back(ping);
        cout << "SONAR "
             << int(PingHeader->Year) << " "
             << int(PingHeader->Month) << " "
             << int(PingHeader->Day) << " "
             << int(PingHeader->Hour) << " "
             << int(PingHeader->Minute) << " "
             << int(PingHeader->Second) << " "
             << int(PingHeader->HSeconds) << " "
             << "Sound vel=" << PingHeader->SoundVelocity << " "
             << "Computed sound vel=" << PingHeader->ComputedSoundVelocity << " "
             << "Y=" << PingHeader->SensorYcoordinate << " "
             << "X=" << PingHeader->SensorXcoordinate << " "
             << "altitude=" << PingHeader->SensorPrimaryAltitude << " "
             << "depth=" << PingHeader->SensorDepth << " "
             << "pitch=" << PingHeader->SensorPitch << " "
             << "roll=" << PingHeader->SensorRoll << " "
             << "heading=" << PingHeader->SensorHeading << " "  // [h] Fish heading in degrees
             << "heave=" << PingHeader->Heave << " "            // Sensor heave at start of ping. 
                           // Positive value means sensor moved up.
             << "yaw=" << PingHeader->Yaw << endl;              // Sensor yaw.  Positive means turn to right.
        cout << ping.time_string_ << endl;
    }

    if (!pings.empty()) {
        pings[0].first_in_file_ = true;
    }

    if (amt == 0xFFFF) {
        cout << "Stopped - read -1 bytes" << endl;
    }

    cout << "Done!" << endl;

    return pings;
}

template <>
xtf_sss_ping::PingsT parse_file(const boost::filesystem::path& file)
{
   xtf_sss_ping::PingsT pings;

   // quick sanity check to make sure the compiler didn't
   // screw up the data structure sizes by changing alignment.
   if (
      sizeof(XTFFILEHEADER)      != 1024 || 
      sizeof(CHANINFO)           != 128  ||
      sizeof(XTFPINGHEADER)      != 256  ||
      sizeof(XTFNOTESHEADER)     != 256  ||
      sizeof(XTFBATHHEADER)      != 256  ||
      sizeof(XTFRAWSERIALHEADER) != 64   ||
      sizeof(XTFPINGCHANHEADER)  != 64
      ) {
         cout << "Error: Bad structure size! " <<
            sizeof(XTFFILEHEADER)       << " " <<
            sizeof(CHANINFO)            << " " <<
            sizeof(XTFPINGHEADER)       << " " <<
            sizeof(XTFNOTESHEADER)      << " " <<
            sizeof(XTFBATHHEADER)       << " " <<
            sizeof(XTFRAWSERIALHEADER)  << " " <<
            sizeof(XTFPINGCHANHEADER)   << endl;
         return pings;
   }

   int infl = open(file.string().c_str(), O_RDONLY, 0000200);
   if (infl <= 0) {
       cout << "Error: Can't open " << file.string() << " for reading!" << endl;
       return pings;
   }

   //
   // Allocate memory for reading file data
   //
   unsigned char* buffer = (unsigned char*)malloc((WORD)32768);
   if (buffer == NULL) {
       cout << "Can't allocate memory!" << endl;
       return pings;
   }

   //
   // Allocate memory for storing XTF header
   //
   XTFFILEHEADER XTFFileHeader; // = (XTFFILEHEADER*)malloc((WORD)sizeof(XTFFILEHEADER));
   pings = read_xtf_file(infl, &XTFFileHeader, buffer);

   if (infl > 0) {
       close(infl);
       infl = 0;
   }
   if (buffer != NULL) {
       free(buffer);
       buffer = NULL;
   }

   return pings;
}