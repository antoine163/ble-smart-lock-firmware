/***
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 antoine163
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Define ----------------------------------------------------------------------
#define BLE_EVENT_DEBUG 1

#if BLE_EVENT_DEBUG == 1
#define BLE_EVENT_PRINT(...) boardDgb(__VA_ARGS__)
#else
#define BLE_EVENT_PRINT(...)
#endif

// Implemented functions -------------------------------------------------------

void hci_disconnection_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Reason)
{
    BLE_EVENT_PRINT("hci_disconnection_complete_event\r\n");
    BLE_EVENT_PRINT("\tStatus:0x%x\r\n", Status);
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tReason:0x%x\r\n", Reason);

    _TASK_BLE_FLAG_CLEAR(CONNECTED);
    _TASK_BLE_FLAG_SET(DO_ADVERTISING);
    taskBleSendEvent(BLE_EVENT_DISCONNECTED);
}

void hci_le_connection_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Role,
    __attribute__((unused)) uint8_t Peer_Address_Type,
    __attribute__((unused)) uint8_t Peer_Address[6],
    __attribute__((unused)) uint16_t Conn_Interval,
    __attribute__((unused)) uint16_t Conn_Latency,
    __attribute__((unused)) uint16_t Supervision_Timeout,
    __attribute__((unused)) uint8_t Master_Clock_Accuracy)
{
#if BLE_EVENT_DEBUG == 1
    BLE_EVENT_PRINT("hci_le_connection_complete_event\r\n");
    BLE_EVENT_PRINT("\tStatus:0x%x\r\n", Status);
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tRole:%s\r\n",
                    (Role == 0x00) ? "Master" : "Slave");
    BLE_EVENT_PRINT("\tPeer_Address_Type:%s\r\n",
                    (Peer_Address_Type == 0x00) ? "Public Device Address" : "Random Device Address");
    BLE_EVENT_PRINT("\tPeer_Address:0x%x%x%x%x%x%x\r\n", Peer_Address[5], Peer_Address[4], Peer_Address[3], Peer_Address[2], Peer_Address[1], Peer_Address[0]);
    BLE_EVENT_PRINT("\tConn_Interval:%u ms\r\n", (int)((float)Conn_Interval * 1.25));
    BLE_EVENT_PRINT("\tConn_Latency:0x%x\r\n", Conn_Latency);
    BLE_EVENT_PRINT("\tSupervision_Timeout:%u ms\r\n", (unsigned int)Supervision_Timeout * 10);

    char *strMaster_Clock_Accuracy = "";
    switch (Master_Clock_Accuracy)
    {
    case 0x00: strMaster_Clock_Accuracy = "500 ppm"; break;
    case 0x01: strMaster_Clock_Accuracy = "250 ppm"; break;
    case 0x02: strMaster_Clock_Accuracy = "150 ppm"; break;
    case 0x03: strMaster_Clock_Accuracy = "100 ppm"; break;
    case 0x04: strMaster_Clock_Accuracy = "75 ppm"; break;
    case 0x05: strMaster_Clock_Accuracy = "50 ppm"; break;
    case 0x06: strMaster_Clock_Accuracy = "30 ppm"; break;
    case 0x07: strMaster_Clock_Accuracy = "20 ppm"; break;
    }
    BLE_EVENT_PRINT("\tMaster_Clock_Accuracy:%s\r\n", strMaster_Clock_Accuracy);
#endif

    _taskBle.connectionHandle = Connection_Handle;
    _TASK_BLE_FLAG_SET(CONNECTED);
    _TASK_BLE_FLAG_SET(DO_SLAVE_SECURITY_REQ);
    taskBleSendEvent(BLE_EVENT_CONNECTED);
}

void hci_le_enhanced_connection_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Role,
    __attribute__((unused)) uint8_t Peer_Address_Type,
    __attribute__((unused)) uint8_t Peer_Address[6],
    __attribute__((unused)) uint8_t Local_Resolvable_Private_Address[6],
    __attribute__((unused)) uint8_t Peer_Resolvable_Private_Address[6],
    __attribute__((unused)) uint16_t Conn_Interval,
    __attribute__((unused)) uint16_t Conn_Latency,
    __attribute__((unused)) uint16_t Supervision_Timeout,
    __attribute__((unused)) uint8_t Master_Clock_Accuracy)
{
#if BLE_EVENT_DEBUG == 1
    BLE_EVENT_PRINT("hci_le_enhanced_connection_complete_event\r\n");
    BLE_EVENT_PRINT("\tStatus:0x%x\r\n", Status);
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tRole:%s\r\n",
                    (Role == 0x00) ? "Master" : "Slave");

    if (Peer_Address_Type == 0x00)
        BLE_EVENT_PRINT("\tPeer_Address_Type:Public Device Address\r\n");
    else if (Peer_Address_Type == 0x01)
        BLE_EVENT_PRINT("\tPeer_Address_Type:Random Device Address\r\n");
    else if (Peer_Address_Type == 0x02)
        BLE_EVENT_PRINT("\tPeer_Address_Type:Public Identity Address\r\n");
    else if (Peer_Address_Type == 0x03)
        BLE_EVENT_PRINT("\tPeer_Address_Type:Random (Static) Identity Address\r\n");

    BLE_EVENT_PRINT("\tPeer_Address:0x%x%x%x%x%x%x\r\n", Peer_Address[5], Peer_Address[4], Peer_Address[3], Peer_Address[2], Peer_Address[1], Peer_Address[0]);

    BLE_EVENT_PRINT("\tLocal_Resolvable_Private_Address:0x%x%x%x%x%x%x\r\n", Local_Resolvable_Private_Address[5], Local_Resolvable_Private_Address[4], Local_Resolvable_Private_Address[3], Local_Resolvable_Private_Address[2], Local_Resolvable_Private_Address[1], Local_Resolvable_Private_Address[0]);
    BLE_EVENT_PRINT("\tPeer_Resolvable_Private_Address:0x%x%x%x%x%x%x\r\n", Peer_Resolvable_Private_Address[5], Peer_Resolvable_Private_Address[4], Peer_Resolvable_Private_Address[3], Peer_Resolvable_Private_Address[2], Peer_Resolvable_Private_Address[1], Peer_Resolvable_Private_Address[0]);
    BLE_EVENT_PRINT("\tConn_Interval:%u ms\r\n", (int)((float)Conn_Interval * 1.25));
    BLE_EVENT_PRINT("\tConn_Latency:0x%x\r\n", Conn_Latency);
    BLE_EVENT_PRINT("\tSupervision_Timeout:%u ms\r\n", (unsigned int)Supervision_Timeout * 10);

    char *strMaster_Clock_Accuracy = "";
    switch (Master_Clock_Accuracy)
    {
    case 0x00: strMaster_Clock_Accuracy = "500 ppm"; break;
    case 0x01: strMaster_Clock_Accuracy = "250 ppm"; break;
    case 0x02: strMaster_Clock_Accuracy = "150 ppm"; break;
    case 0x03: strMaster_Clock_Accuracy = "100 ppm"; break;
    case 0x04: strMaster_Clock_Accuracy = "75 ppm"; break;
    case 0x05: strMaster_Clock_Accuracy = "50 ppm"; break;
    case 0x06: strMaster_Clock_Accuracy = "30 ppm"; break;
    case 0x07: strMaster_Clock_Accuracy = "20 ppm"; break;
    }
    BLE_EVENT_PRINT("\tMaster_Clock_Accuracy:%s\r\n", strMaster_Clock_Accuracy);
#endif

    _taskBle.connectionHandle = Connection_Handle;
    _TASK_BLE_FLAG_SET(CONNECTED);
    _TASK_BLE_FLAG_SET(DO_SLAVE_SECURITY_REQ);
    taskBleSendEvent(BLE_EVENT_CONNECTED);
}

void aci_gap_pairing_complete_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint8_t Reason)
{
#if BLE_EVENT_DEBUG == 1
    BLE_EVENT_PRINT("aci_gap_pairing_complete_event\r\n");
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);

    char *strStatus = "";
    char *strReason = "";

    switch (Status)
    {
    case 0x00: strStatus = "Success"; break;
    case 0x01: strStatus = "Timeout"; break;
    case 0x02: strStatus = "Pairing Failed"; break;
    case 0x03: strStatus = "Encryption failed, LTK missing on local device"; break;
    case 0x04: strStatus = "Encryption failed, LTK missing on peer device"; break;
    case 0x05: strStatus = "Encryption not supported by remote device"; break;
    }

    BLE_EVENT_PRINT("\t Status:%s\r\n", strStatus);

    if (Status == 0x02)
    {
        switch (Reason)
        {
        case 0x00: strReason = ""; break;
        case 0x01: strReason = "PASSKEY_ENTRY_FAILED"; break;
        case 0x02: strReason = "OOB_NOT_AVAILABLE"; break;
        case 0x03: strReason = "AUTH_REQ_CANNOT_BE_MET"; break;
        case 0x04: strReason = "CONFIRM_VALUE_FAILED"; break;
        case 0x05: strReason = "PAIRING_NOT_SUPPORTED"; break;
        case 0x06: strReason = "INSUFF_ENCRYPTION_KEY_SIZE"; break;
        case 0x07: strReason = "CMD_NOT_SUPPORTED"; break;
        case 0x08: strReason = "UNSPECIFIED_REASON"; break;
        case 0x09: strReason = "VERY_EARLY_NEXT_ATTEMPT"; break;
        case 0x0A: strReason = "SM_INVALID_PARAMS"; break;
        case 0x0B: strReason = "SMP_SC_DHKEY_CHECK_FAILED"; break;
        case 0x0C: strReason = "SMP_SC_NUMCOMPARISON_FAILED"; break;
        }

        BLE_EVENT_PRINT("\t Reason:%s\r\n", strReason);
    }
#endif

    // If success and bonding mode
    if ((Status == 0x00) && _TASK_BLE_FLAG_IS(BONDING))
        _TASK_BLE_FLAG_SET(DO_CONFIGURE_WHITELIST);

    _TASK_BLE_FLAG_CLEAR(BONDING);
}

void aci_gatt_attribute_modified_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attr_Handle,
    __attribute__((unused)) uint16_t Offset,
    __attribute__((unused)) uint16_t Attr_Data_Length,
    __attribute__((unused)) uint8_t Attr_Data[])
{
    BLE_EVENT_PRINT("aci_gatt_attribute_modified_event\r\n");
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tAttr_Handle:0x%x\r\n", Attr_Handle);
    BLE_EVENT_PRINT("\tOffset:%u\r\n", Offset);
    BLE_EVENT_PRINT("\tAttr_Data_Length:%u\r\n", Attr_Data_Length);

    // 'Attribute profile service' from 0x0001 to 0x0004
    // Service changed
    if (Attr_Handle == 0x0002)
    {
    }

    // 'Generic access profile (GAP) service' from 0x0005 to 0x000b
    // Device name
    else if (Attr_Handle == 0x0006)
    {
    }
    // Appearance
    else if (Attr_Handle == 0x0008)
    {
    }
    // Peripheral preferred connection parameters
    else if (Attr_Handle == 0x000a)
    {
    }
    // Central address resolution
    // It is added only when controller-based privacy (0x02) is enabled on aci_gap_init() API
    else if (Attr_Handle == 0x000c)
    {
    }

    // -- Application attributs --
    // Commend to unlock lock
    else if (Attr_Handle == _taskBle.lockStateCharAppHandle + 1)
    {
        if (Attr_Data[0] == 0x01)
            taskAppUnlock();
    }
    // Commend to open door
    else if (Attr_Handle == _taskBle.openDoorCharAppHandle + 1)
    {
        if (Attr_Data[0] == 0x01)
            taskAppOpenDoor();
    }
    // Commend to set brightness threshold
    else if (Attr_Handle == _taskBle.brightnessThCharAppHandle + 1)
    {
        taskAppSetBrightnessTh(*((float *)Attr_Data));
    }
}

void aci_gatt_read_permit_req_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint16_t Offset)
{
    BLE_EVENT_PRINT("aci_gatt_read_permit_req_event\r\n");
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tAttribute_Handle:0x%x\r\n", Attribute_Handle);
    BLE_EVENT_PRINT("\tOffset:%u\r\n", Offset);

    _TASK_BLE_FLAG_SET(DO_NOTIFY_READ_REQ);
}

void aci_hal_end_of_radio_activity_event(
    __attribute__((unused)) uint8_t Last_State,
    __attribute__((unused)) uint8_t Next_State,
    __attribute__((unused)) uint32_t Next_State_SysTime)
{
    // BLE_EVENT_PRINT("aci_hal_end_of_radio_activity_event\r\n");
    // BLE_EVENT_PRINT("\tLast_State:%u\r\n", Last_State);
    // BLE_EVENT_PRINT("\tNext_State:%u\r\n", Next_State);
    // BLE_EVENT_PRINT("\tNext_State_SysTime:%u\r\n", Next_State_SysTime);

    // BLE_EVENT_PRINT("\tms:%u\r\n",
    //     HAL_VTimerDiff_ms_sysT32(Next_State_SysTime, HAL_VTimerGetCurrentTime_sysT32()));

    _taskBle.nextStateSysTime = Next_State_SysTime;
}

void hci_hardware_error_event(
    __attribute__((unused)) uint8_t Hardware_Code)
{
    BLE_EVENT_PRINT("hci_hardware_error_event\r\n");
    taskBleSendEvent(BLE_EVENT_ERR);
}

void hci_encryption_change_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Encryption_Enabled)
{
    BLE_EVENT_PRINT("hci_encryption_change_event\r\n");
    BLE_EVENT_PRINT("\tStatus:0x%x\r\n", Status);
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tEncryption_Enabled:%s\r\n",
                    (Encryption_Enabled = 0x00) ? "Link Level Encryption OFF" : "Link Level Encryption is ON with AES-CCM");
}

void hci_read_remote_version_information_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Version,
    __attribute__((unused)) uint16_t Manufacturer_Name,
    __attribute__((unused)) uint16_t Subversion)
{
    BLE_EVENT_PRINT("hci_read_remote_version_information_complete_event\r\n");
}

void hci_number_of_completed_packets_event(
    __attribute__((unused)) uint8_t Number_of_Handles,
    __attribute__((unused)) Handle_Packets_Pair_Entry_t Handle_Packets_Pair_Entry[])
{
    BLE_EVENT_PRINT("hci_number_of_completed_packets_event\r\n");
}

void hci_data_buffer_overflow_event(
    __attribute__((unused)) uint8_t Link_Type)
{
    BLE_EVENT_PRINT("hci_data_buffer_overflow_event\r\n");
}

void hci_encryption_key_refresh_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("hci_encryption_key_refresh_complete_event\r\n");
}

tBleStatus hci_rx_acl_data_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t PB_Flag,
    __attribute__((unused)) uint8_t BC_Flag,
    __attribute__((unused)) uint16_t Data_Length,
    __attribute__((unused)) uint8_t *PDU_Data)
{
    BLE_EVENT_PRINT("hci_rx_acl_data_event\r\n");

    return BLE_STATUS_SUCCESS;
}

void hci_le_advertising_report_event(
    __attribute__((unused)) uint8_t Num_Reports,
    __attribute__((unused)) Advertising_Report_t Advertising_Report[])
{
    BLE_EVENT_PRINT("hci_le_advertising_report_event\r\n");
}

void hci_le_connection_update_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Conn_Interval,
    __attribute__((unused)) uint16_t Conn_Latency,
    __attribute__((unused)) uint16_t Supervision_Timeout)
{
    BLE_EVENT_PRINT("hci_le_connection_update_complete_event\r\n");
    BLE_EVENT_PRINT("\tStatus:0x%x\r\n", Status);
    BLE_EVENT_PRINT("\tConnection_Handle:0x%x\r\n", Connection_Handle);
    BLE_EVENT_PRINT("\tConn_Interval:%u ms\r\n", (int)((float)Conn_Interval * 1.25));
    BLE_EVENT_PRINT("\tConn_Latency:0x%x\r\n", Conn_Latency);
    BLE_EVENT_PRINT("\tSupervision_Timeout:%u ms\r\n", (unsigned int)Supervision_Timeout * 10);
}

void hci_le_read_remote_used_features_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t LE_Features[8])
{
    BLE_EVENT_PRINT("hci_le_read_remote_used_features_complete_event\r\n");
}

void hci_le_long_term_key_request_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Random_Number[8],
    __attribute__((unused)) uint16_t Encrypted_Diversifier)
{
    BLE_EVENT_PRINT("hci_le_long_term_key_request_event\r\n");
}

void hci_le_data_length_change_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t MaxTxOctets,
    __attribute__((unused)) uint16_t MaxTxTime,
    __attribute__((unused)) uint16_t MaxRxOctets,
    __attribute__((unused)) uint16_t MaxRxTime)
{
    BLE_EVENT_PRINT("hci_le_data_length_change_event\r\n");
}

void hci_le_read_local_p256_public_key_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint8_t Local_P256_Public_Key[64])
{
    BLE_EVENT_PRINT("hci_le_read_local_p256_public_key_complete_event\r\n");
}

void hci_le_generate_dhkey_complete_event(
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint8_t DHKey[32])
{
    BLE_EVENT_PRINT("hci_le_generate_dhkey_complete_event\r\n");
}

void hci_le_direct_advertising_report_event(
    __attribute__((unused)) uint8_t Num_Reports,
    __attribute__((unused)) Direct_Advertising_Report_t Direct_Advertising_Report[])
{
    BLE_EVENT_PRINT("hci_le_direct_advertising_report_event\r\n");
}

void aci_gap_limited_discoverable_event(void)
{
    BLE_EVENT_PRINT("aci_gap_limited_discoverable_event\r\n");
}

void aci_gap_pass_key_req_event(
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("aci_gap_pass_key_req_event\r\n");
}

void aci_gap_authorization_req_event(
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("aci_gap_authorization_req_event\r\n");
}

void aci_gap_slave_security_initiated_event(void)
{
    BLE_EVENT_PRINT("aci_gap_slave_security_initiated_event\r\n");
}

void aci_gap_bond_lost_event(void)
{
    BLE_EVENT_PRINT("aci_gap_bond_lost_event\r\n");
}

void aci_gap_proc_complete_event(
    __attribute__((unused)) uint8_t Procedure_Code,
    __attribute__((unused)) uint8_t Status,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Data[])
{
    BLE_EVENT_PRINT("aci_gap_proc_complete_event\r\n");
}

void aci_gap_addr_not_resolved_event(
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("aci_gap_addr_not_resolved_event\r\n");
}

void aci_gap_numeric_comparison_value_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint32_t Numeric_Value)
{
    BLE_EVENT_PRINT("aci_gap_numeric_comparison_value_event\r\n");
    BLE_EVENT_PRINT("\tNumeric_Value:%u\r\n", Numeric_Value);
}

void aci_gap_keypress_notification_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Notification_Type)
{
    BLE_EVENT_PRINT("aci_gap_keypress_notification_event\r\n");
}

void aci_gatt_proc_timeout_event(
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("aci_gatt_proc_timeout_event\r\n");
}

void aci_att_exchange_mtu_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Server_RX_MTU)
{
    BLE_EVENT_PRINT("aci_att_exchange_mtu_resp_event\r\n");
}

void aci_att_find_info_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Format,
    __attribute__((unused)) uint8_t Event_Data_Length,
    __attribute__((unused)) uint8_t Handle_UUID_Pair[])
{
    BLE_EVENT_PRINT("aci_att_find_info_resp_event\r\n");
}

void aci_att_find_by_type_value_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Num_of_Handle_Pair,
    __attribute__((unused)) Attribute_Group_Handle_Pair_t Attribute_Group_Handle_Pair[])
{
    BLE_EVENT_PRINT("aci_att_find_by_type_value_resp_event\r\n");
}

void aci_att_read_by_type_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Handle_Value_Pair_Length,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Handle_Value_Pair_Data[])
{
    BLE_EVENT_PRINT("aci_att_read_by_type_resp_event\r\n");
}

void aci_att_read_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Event_Data_Length,
    __attribute__((unused)) uint8_t Attribute_Value[])
{
    BLE_EVENT_PRINT("aci_att_read_resp_event\r\n");
}

void aci_att_read_blob_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Event_Data_Length,
    __attribute__((unused)) uint8_t Attribute_Value[])
{
    BLE_EVENT_PRINT("aci_att_read_blob_resp_event\r\n");
}

void aci_att_read_multiple_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Event_Data_Length,
    __attribute__((unused)) uint8_t Set_Of_Values[])
{
    BLE_EVENT_PRINT("aci_att_read_multiple_resp_event\r\n");
}

void aci_att_read_by_group_type_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Attribute_Data_Length,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Attribute_Data_List[])
{
    BLE_EVENT_PRINT("aci_att_read_by_group_type_resp_event\r\n");
}

void aci_att_prepare_write_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint16_t Offset,
    __attribute__((unused)) uint8_t Part_Attribute_Value_Length,
    __attribute__((unused)) uint8_t Part_Attribute_Value[])
{
    BLE_EVENT_PRINT("aci_att_prepare_write_resp_event\r\n");
}

void aci_att_exec_write_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("aci_att_exec_write_resp_event\r\n");
}

void aci_gatt_indication_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint8_t Attribute_Value_Length,
    __attribute__((unused)) uint8_t Attribute_Value[])
{
    BLE_EVENT_PRINT("aci_gatt_indication_event\r\n");
}

void aci_gatt_notification_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint8_t Attribute_Value_Length,
    __attribute__((unused)) uint8_t Attribute_Value[])
{
    BLE_EVENT_PRINT("aci_gatt_notification_event\r\n");
}

void aci_gatt_proc_complete_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Error_Code)
{
    BLE_EVENT_PRINT("aci_gatt_proc_complete_event\r\n");
}

void aci_gatt_error_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Req_Opcode,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint8_t Error_Code)
{
    BLE_EVENT_PRINT("aci_gatt_error_resp_event\r\n");
}

void aci_gatt_disc_read_char_by_uuid_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint8_t Attribute_Value_Length,
    __attribute__((unused)) uint8_t Attribute_Value[])
{
    BLE_EVENT_PRINT("aci_gatt_disc_read_char_by_uuid_resp_event\r\n");
}

void aci_gatt_write_permit_req_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Data[])
{
    BLE_EVENT_PRINT("aci_gatt_write_permit_req_event\r\n");
}

void aci_gatt_read_multi_permit_req_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Number_of_Handles,
    __attribute__((unused)) Handle_Item_t Handle_Item[])
{
    BLE_EVENT_PRINT("aci_gatt_read_multi_permit_req_event\r\n");
}

void aci_gatt_tx_pool_available_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Available_Buffers)
{
    BLE_EVENT_PRINT("aci_gatt_tx_pool_available_event\r\n");
}

void aci_gatt_server_confirmation_event(
    __attribute__((unused)) uint16_t Connection_Handle)
{
    BLE_EVENT_PRINT("aci_gatt_server_confirmation_event\r\n");
}

void aci_gatt_prepare_write_permit_req_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Attribute_Handle,
    __attribute__((unused)) uint16_t Offset,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Data[])
{
    BLE_EVENT_PRINT("aci_gatt_prepare_write_permit_req_event\r\n");
}

void aci_l2cap_connection_update_resp_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint16_t Result)
{
    BLE_EVENT_PRINT("aci_l2cap_connection_update_resp_event\r\n");
}

void aci_l2cap_proc_timeout_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Data[])
{
    BLE_EVENT_PRINT("aci_l2cap_proc_timeout_event\r\n");
}

void aci_l2cap_connection_update_req_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Identifier,
    __attribute__((unused)) uint16_t L2CAP_Length,
    __attribute__((unused)) uint16_t Interval_Min,
    __attribute__((unused)) uint16_t Interval_Max,
    __attribute__((unused)) uint16_t Slave_Latency,
    __attribute__((unused)) uint16_t Timeout_Multiplier)
{
    BLE_EVENT_PRINT("aci_l2cap_connection_update_req_event\r\n");
}

void aci_l2cap_command_reject_event(
    __attribute__((unused)) uint16_t Connection_Handle,
    __attribute__((unused)) uint8_t Identifier,
    __attribute__((unused)) uint16_t Reason,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Data[])
{
    BLE_EVENT_PRINT("aci_l2cap_command_reject_event\r\n");
}

void aci_hal_scan_req_report_event(
    __attribute__((unused)) int8_t RSSI,
    __attribute__((unused)) uint8_t Peer_Address_Type,
    __attribute__((unused)) uint8_t Peer_Address[6])
{
    BLE_EVENT_PRINT("aci_hal_scan_req_report_event\r\n");
}

void aci_hal_fw_error_event(
    __attribute__((unused)) uint8_t FW_Error_Type,
    __attribute__((unused)) uint8_t Data_Length,
    __attribute__((unused)) uint8_t Data[])
{
    BLE_EVENT_PRINT("aci_hal_fw_error_event\r\n");
}
