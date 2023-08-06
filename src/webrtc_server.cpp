#include "webrtc_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "defjams.h"
#include "nlohmann/json.hpp"
#include "rtc/rtc.hpp"
#include "tsqueue.hpp"

extern Tsqueue<StereoVal> webrtc_output_queue;

using nlohmann::json;
typedef int SOCKET;

void *web_rtc_server_thread() {
  rtc::InitLogger(rtc::LogLevel::Debug);
  auto pc = std::make_shared<rtc::PeerConnection>();

  pc->onStateChange([](rtc::PeerConnection::State state) {
    std::cout << "WebRTC State: " << state << std::endl;
  });

  pc->onGatheringStateChange([pc](rtc::PeerConnection::GatheringState state) {
    std::cout << "WebRTC Gathering State: " << state << std::endl;
    if (state == rtc::PeerConnection::GatheringState::Complete) {
      auto description = pc->localDescription();
      json message = {{"type", description->typeString()},
                      {"sdp", std::string(description.value())}};
      std::cout << message << std::endl;
    }
  });

  std::cout << "WEB RTC SERVER THRED STARTED!\n";

  const rtc::SSRC ssrc = 42;
  rtc::Description::Video media("video", rtc::Description::Direction::SendOnly);
  media.addH264Codec(
      96);  // Must match the payload type of the external h264 RTP stream
  media.addSSRC(ssrc, "video-send");
  auto track = pc->addTrack(media);

  pc->setLocalDescription();

  std::cout << "Please copy/paste the answer provided by the browser: "
            << std::endl;
  std::string sdp;
  std::getline(std::cin, sdp);

  json j = json::parse(sdp);
  rtc::Description answer(j["sdp"].get<std::string>(),
                          j["type"].get<std::string>());
  pc->setRemoteDescription(answer);

  while (auto sval = webrtc_output_queue.pop()) {
    if (sval) {
      if (sval->left == -99 && sval->right == -99) {
        std::cout << "GOT EXIT SIGN\n";
        break;
      }
      //  auto rtp = reinterpret_cast<rtc::RtpHeader *>(sval);
      //  rtp->setSsrc(ssrc);

      //  track->send(reinterpret_cast<const std::byte *>(sval), 2);
    }
  }

  return nullptr;
}
