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
#pragma once // ProtoUtils.h
#include "Logger.h"
#include "Blob.h"
#include <google/protobuf/message.h>
#include <google/protobuf/map.h>
#include <google/protobuf/repeated_field.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace LiveKitCpp
{

std::vector<uint8_t> protoToBytes(const google::protobuf::Message& proto,
                                  Bricks::Logger* logger = nullptr,
                                  std::string_view category = {});

template <typename TProto>
inline std::optional<TProto> protoFromBytes(const void* data,
                                            size_t dataLen,
                                            Bricks::Logger* logger = nullptr,
                                            std::string_view category = {}) {
    static_assert(std::is_base_of_v<google::protobuf::Message, TProto>);
    if (data && dataLen) {
        TProto proto;
        if (proto.ParseFromArray(data, int(dataLen))) {
            return proto;
        }
        if (logger && logger->canLogError()) {
            logger->logError(std::string("failed parse of ") +
                             proto.GetTypeName() + " from blob", category);
        }
    }
    return std::nullopt;
}

template <typename TProto>
inline std::optional<TProto> protoFromBytes(const Bricks::Blob& blob,
                                            Bricks::Logger* logger = nullptr,
                                            std::string_view category = {}) {
    return protoFromBytes<TProto>(blob.data(), blob.size(), logger, category);
}


template<typename KIn, typename VIn, typename KOut, typename VOut, class KConv, class VConv>
inline void toProtoMap(const std::unordered_map<KIn, VIn>& from,
                       google::protobuf::Map<KOut, VOut>* to,
                       KConv kConv, VConv vConv)
{
    if (to) {
        for (auto it = from.begin(); it != from.end(); ++it) {
            to->insert({kConv(it->first), vConv(it->second)});
        }
    }
}

template<typename K, typename V>
inline void toProtoMap(const std::unordered_map<K, V>& from,
                       google::protobuf::Map<K, V>* to)
{
    if (to) {
        toProtoMap<K, V, K, V>(from, to,
                               [](const K& val) -> const K& { return val; },
                               [](const V& val) -> const V& { return val; });
    }
}

template<typename KIn, typename VIn, typename KOut, typename VOut, class KConv, class VConv>
inline std::unordered_map<KOut, VOut> fromProtoMap(const google::protobuf::Map<KIn, VIn>& in,
                                                   KConv kConv, VConv vConv)
{
    if (const auto size = in.size()) {
        std::unordered_map<KOut, VOut> out;
        out.reserve(size);
        for (auto it = in.begin(); it != in.end(); ++it) {
            out[kConv(it->first)] = vConv(it->second);
        }
        return out;
    }
    return {};
}

template<typename K, typename V>
inline std::unordered_map<K, V> fromProtoMap(const google::protobuf::Map<K, V>& in) {
    return fromProtoMap<K, V, K, V>(in,
                                    [](const K& val) -> const K& { return val; },
                                    [](const V& val) -> const V& { return val; });
}

template<typename TIn, typename TProtoBufRepeated, class Conv>
inline void toProtoRepeated(const std::vector<TIn>& in,
                            TProtoBufRepeated* out,
                            Conv conv) {
    if (out) {
        if (const auto size = in.size()) {
            out->Reserve(int(out->size() + size));
            for (size_t i = 0U; i < size; ++i) {
                *out->Add() = conv(in[i]);
            }
        }
    }
}

template<typename TIn, typename TProtoBufRepeated>
inline void toProtoRepeated(const std::vector<TIn>& in,
                            TProtoBufRepeated* out) {
    if (out) {
        toProtoRepeated<TIn, TProtoBufRepeated>(in, out, [](const TIn& val) ->
                                                const TIn& { return val; });
    }
}

template<typename TOut, typename TProtoBufRepeated, class Conv>
inline std::vector<TOut> fromProtoRepeated(const TProtoBufRepeated& in,
                                           Conv conv) {
    std::vector<TOut> out;
    if (const auto size = in.size()) {
        out.reserve(size_t(size));
        for (const auto& val : in) {
            out.push_back(conv(val));
        }
    }
    return out;
}

template<typename TOut, typename TProtoBufRepeated>
inline std::vector<TOut> fromProtoRepeated(const TProtoBufRepeated& in) {
    return fromProtoRepeated<TOut, TProtoBufRepeated>(in, [](const auto& val) { return val; });
}


} // namespace LiveKitCpp
