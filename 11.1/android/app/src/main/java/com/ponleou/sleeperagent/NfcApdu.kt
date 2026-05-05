package com.ponleou.sleeperagent

object NfcApdu {
    const val CLA_STANDARD: Int = 0x00
    const val CLA_PROPRIETARY: Int = 0x80

    const val INS_SELECT: Int = 0xA4
    const val INS_IDENTIFY: Int = 0x01
    const val INS_POLL: Int = 0x02
    const val INS_COLLECT: Int = 0x03
    const val INS_START_CLIENT_COLLECTOR: Int = 0x04
    const val INS_STOP_CLIENT_COLLECTOR: Int = 0x05

    const val P1_SELECT_BY_AID: Int = 0x04
    const val P2_FIRST_OR_ONLY: Int = 0x00

    const val P_NULL: Int = 0x00
    const val LE_ID_LENGTH: Int = 0x06

    const val P1_COLLECT_PROVIDE_LENGTH: Int = 0x01
    const val P2_COLLECT_LENGTH_POS: Int = 0x00

    const val MAX_APDU_RESPONSE_BYTES: Int = 255
    const val COLLECT_OVERHEAD_BYTES: Int = 3
    const val MAX_COLLECT_DATA_BYTES: Int = MAX_APDU_RESPONSE_BYTES - COLLECT_OVERHEAD_BYTES

    val AID_SLEEPER_AGENT: ByteArray = byteArrayOf(
        0xF0.toByte(),
        0x73.toByte(),
        0x6C.toByte(),
        0x65.toByte(),
        0x65.toByte(),
        0x70.toByte(),
        0x65.toByte(),
        0x72.toByte(),
        0x20.toByte(),
        0x61.toByte(),
        0x67.toByte(),
        0x65.toByte(),
        0x6E.toByte(),
        0x74.toByte()
    )

    val SW_SUCCESS: ByteArray = byteArrayOf(0x90.toByte(), 0x00.toByte())
    val SW_DATA: ByteArray = byteArrayOf(0x61.toByte(), 0x00.toByte())
    val SW_UNKNOWN: ByteArray = byteArrayOf(0x6A.toByte(), 0x82.toByte())
}
