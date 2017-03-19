#include <iostream>

#define LINK_PLATFORM_MACOSX 1

#include <ableton/Link.hpp>

#include "ableton_link_wrapper.h"

struct AbletonLink
{
  std::atomic<bool> running;
  ableton::Link link;
  //ableton::linkaudio::AudioPlatform audioPlatform;

  AbletonLink()
    : running(true)
    , link(120.)
    //, audioPlatform(link)
  {
    link.enable(true);
  }
};

extern "C" {
    AbletonLink *new_ableton_link()
    {
        std::cout << "New Ableton Link object!" << std::endl;
        return new AbletonLink();
    }
}

