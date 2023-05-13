#include "modules/rtp_rtcp/rtcp_packet/report_block.h"

#include <modules/rtp_rtcp/source/byte_io.h>
#include <rtc_base/logging.h>

namespace xrtc {
namespace rtcp {

// From RFC 3550, RTP: A Transport Protocol for Real-Time Applications.
//
// RTCP report block (RFC 3550).
//
//     0                   1                   2                   3
//     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//  0 |                 SSRC_1 (SSRC of first source)                 |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 | fraction lost |       cumulative number of packets lost       |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |           extended highest sequence number received           |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 12 |                      interarrival jitter                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                         last SR (LSR)                         |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 20 |                   delay since last SR (DLSR)                  |
// 24 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
bool ReportBlock::Parse(const uint8_t* buffer, size_t len) {
    if (len < kLength) {
        return false;
    }

    source_ssrc_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&buffer[0]);
    fraction_lost_ = buffer[4];
    cumulative_packets_lost_ = webrtc::ByteReader<int32_t, 3>::ReadBigEndian(&buffer[5]);
    extended_high_seq_num_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&buffer[8]);
    jitter_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&buffer[12]);
    last_sr_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&buffer[16]);
    delay_since_last_sr_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&buffer[20]);

    return true;
}

void ReportBlock::Create(uint8_t* buffer) const {
    // Runtime check should be done while setting cumulative_lost.
    RTC_DCHECK_LT(cumulative_packets_lost_, (1 << 23));  // Have only 3 bytes for it.

    webrtc::ByteWriter<uint32_t>::WriteBigEndian(&buffer[0], source_ssrc_);
    webrtc::ByteWriter<uint8_t>::WriteBigEndian(&buffer[4], fraction_lost_);
    webrtc::ByteWriter<int32_t, 3>::WriteBigEndian(&buffer[5], cumulative_packets_lost_);
    webrtc::ByteWriter<uint32_t>::WriteBigEndian(&buffer[8], extended_high_seq_num_);
    webrtc::ByteWriter<uint32_t>::WriteBigEndian(&buffer[12], jitter_);
    webrtc::ByteWriter<uint32_t>::WriteBigEndian(&buffer[16], last_sr_);
    webrtc::ByteWriter<uint32_t>::WriteBigEndian(&buffer[20], delay_since_last_sr_);
}

bool ReportBlock::SetCumulativeLost(int32_t cumulative_lost) {
  // We have only 3 bytes to store it, and it's a signed value.
  if (cumulative_lost >= (1 << 23) || cumulative_lost < -(1 << 23)) {
    RTC_LOG(LS_WARNING) << "Cumulative lost is too big to fit into Report Block";
    return false;
  }

  cumulative_packets_lost_ = cumulative_lost;
  return true;
}

uint32_t ReportBlock::cumulative_lost() const {
  if (cumulative_packets_lost_ < 0) {
    RTC_LOG(LS_VERBOSE) << "Ignoring negative value of cumulative_lost";
    return 0;
  }
  return cumulative_packets_lost_;
}

} // namespace rtcp
} // namespace xrtc