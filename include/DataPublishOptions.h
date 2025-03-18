// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once // DataPublishOptions.h
#include <string>
#include <vector>

namespace LiveKitCpp
{

struct DataPublishOptions
{
    /**
      * whether to send this as reliable or lossy.
      * For data that you need delivery guarantee (such as chat messages), use Reliable.
      * For data that should arrive as quickly as possible, but you are ok with dropped
      * packets, use Lossy.
      */
    bool _reliable = true;
    /**
      * the identities of participants who will receive the message, will be sent to every one if empty
      */
    std::vector<std::string> _destinationIdentities;
    /** the topic under which the message gets published */
    std::string _topic;
};

} // namespace LiveKitCpp
