// Copyright 2020 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "core/ecc.h"

namespace beam::wallet
{
    struct Notification
    {
        enum class Type : uint32_t
        {
            SoftwareUpdateAvailable,
            AddressStatusChanged,
            TransactionStatusChanged,
            BeamNews
        };

        enum class State : uint32_t
        {
            Unread,
            Read,
            Deleted
        };

        // unique ID - probably same as BroadcastMsg::m_signature underlying type
        ECC::uintBig m_ID;
        Type m_type;
        State m_state;
        Timestamp m_createTime;
        ByteBuffer m_content;
    };
} // namespace beam::wallet
