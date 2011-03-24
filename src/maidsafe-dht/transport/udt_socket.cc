/* Copyright (c) 2010 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "maidsafe-dht/transport/udt_socket.h"

#include <utility>

#include "maidsafe/common/log.h"

namespace asio = boost::asio;
namespace bs = boost::system;
namespace bptime = boost::posix_time;
namespace arg = std::placeholders;

namespace maidsafe {

namespace transport {

UdtSocket::UdtSocket(const std::shared_ptr<UdtMultiplexer> &udt_multiplexer,
                     asio::io_service &asio_service,
                     boost::uint32_t id,
                     const Endpoint &remote_endpoint)
  : multiplexer_(udt_multiplexer),
    remote_endpoint_(remote_endpoint),
    waiting_connect_(asio_service),
    waiting_connect_ec_(),
    waiting_write_(asio_service),
    waiting_write_ec_(),
    waiting_write_bytes_transferred_(0),
    waiting_read_(asio_service),
    waiting_read_ec_(),
    waiting_read_bytes_transferred_(0) {
  waiting_connect_.expires_at(boost::posix_time::pos_infin);
  waiting_write_.expires_at(boost::posix_time::pos_infin);
  waiting_read_.expires_at(boost::posix_time::pos_infin);
}

UdtSocket::~UdtSocket() {
}

void UdtSocket::Close() {
  waiting_connect_ec_ = asio::error::operation_aborted;
  waiting_connect_.cancel();
  waiting_write_ec_ = asio::error::operation_aborted;
  waiting_write_bytes_transferred_ = 0;
  waiting_write_.cancel();
  waiting_read_ec_ = asio::error::operation_aborted;
  waiting_read_bytes_transferred_ = 0;
  waiting_read_.cancel();
}

void UdtSocket::StartConnect() {
}

void UdtSocket::StartWrite(const asio::const_buffer &data) {
  // Check for a no-op write.
  if (asio::buffer_size(data) == 0) {
    waiting_write_ec_.clear();
    waiting_write_.cancel();
    return;
  }

  // Try processing the write immediately. If there's space in the write buffer
  // then the operation will complete immediately. Otherwise, it will wait until
  // some other event frees up space in the buffer.
  waiting_write_buffer_ = data;
  waiting_write_bytes_transferred_ = 0;
  ProcessWrite();
}

void UdtSocket::ProcessWrite() {
  // There's only a waiting write if the write buffer is non-empty.
  if (asio::buffer_size(waiting_write_buffer_) == 0)
    return;

  // If the write buffer is full then the write is going to have to wait.
  if (write_buffer_.size() == kMaxWriteBufferSize)
    return;


  // Copy whatever data we can into the write buffer.
  size_t length = std::min(kMaxWriteBufferSize - write_buffer_.size(),
                           asio::buffer_size(waiting_write_buffer_));
  const unsigned char* data =
    asio::buffer_cast<const unsigned char*>(waiting_write_buffer_);
  write_buffer_.insert(write_buffer_.end(), data, data + length);
  waiting_write_buffer_ = waiting_write_buffer_ + length;
  waiting_write_bytes_transferred_ += length;

  // If we have finished writing all of the data then it's time to trigger the
  // write's completion handler.
  if (asio::buffer_size(waiting_write_buffer_) == 0)
  {
    // The write is done. Trigger the write's completion handler.
    waiting_write_ec_.clear();
    waiting_write_.cancel();
  }
}

void UdtSocket::StartRead(const asio::mutable_buffer &data) {
  // Check for a no-read write.
  if (asio::buffer_size(data) == 0) {
    waiting_read_ec_.clear();
    waiting_read_.cancel();
    return;
  }

  // Try processing the read immediately. If there's available data then the
  // operation will complete immediately. Otherwise it will wait until the next
  // data packet arrives.
  waiting_read_buffer_ = data;
  waiting_read_bytes_transferred_ = 0;
  ProcessRead();
}

void UdtSocket::ProcessRead() {
  // There's only a waiting read if the read buffer is non-empty.
  if (asio::buffer_size(waiting_read_buffer_) == 0)
    return;

  // TODO check for data and trigger read handler.
}

void UdtSocket::HandleReceiveFrom(const asio::const_buffer &data,
                                  const asio::ip::udp::endpoint &endpoint) {
}

}  // namespace transport

}  // namespace maidsafe
