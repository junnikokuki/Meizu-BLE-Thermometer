"""
  Parse hci commands
"""
import struct


"""
OpCodes and names for HCI commands according to the Bluetooth specification

OpCode is 2 bytes, of which OGF is the upper 6 bits and OCF is the lower 10 bits.

 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
-------------------------------------------------
|            opcode             |    length     |
-------------------------------------------------
|        OCF        |    OGF    |
---------------------------------
"""
HCI_COMMANDS = {
        0x0401 : "COMND Inquiry",
        0x0402 : "COMND Inquiry_Cancel",
        0x0403 : "COMND Periodic_Inquiry_Mode",
        0x0404 : "COMND Exit_Periodic_Inquiry_Mode",
        0x0405 : "COMND Create_Connection",
        0x0406 : "COMND Disconnect",
        0x0408 : "COMND Create_Connection_Cancel",
        0x0409 : "COMND Accept_Connection_Request",
        0x040a : "COMND Reject_Connection_Request",
        0x040b : "COMND Link_Key_Request_Reply",
        0x040c : "COMND Link_Key_Request_Negative_Reply",
        0x040d : "COMND PIN_Code_Request_Reply",
        0x040e : "COMND PIN_Code_Request_Negative_Reply",
        0x040f : "COMND Change_Connection_Packet_Type",
        0x0411 : "COMND Authentication_Requested",
        0x0413 : "COMND Set_Connection_Encryption ",
        0x0415 : "COMND Change_Connection_Link_Key",
        0x0417 : "COMND Master_Link_Key",
        0x0419 : "COMND Remote_Name_Request",
        0x041a : "COMND Remote_Name_Request_Cancel",
        0x041b : "COMND Read_Remote_Supported_Features",
        0x041c : "COMND Read_Remote_Extended_Features",
        0x041d : "COMND Read_Remote_Version_Information",
        0x041f : "COMND Read_Clock_Offset",
        0x0420 : "COMND Read_LMP_Handle",
        0x0428 : "COMND Setup_Synchronous_Connection",
        0x0429 : "COMND Accept_Synchronous_Connection_Request",
        0x042a : "COMND Reject_Synchronous_Connection_Request",
        0x042b : "COMND IO_Capability_Request_Reply",
        0x042c : "COMND User_Confirmation_Request_Reply",
        0x042d : "COMND User_Confirmation_Request_Negative_Reply",
        0x042e : "COMND User_Passkey_Request_Reply",
        0x042f : "COMND User_Passkey_Request_Negative_Reply",
        0x0430 : "COMND Remote_OOB_Data_Request_Reply",
        0x0433 : "COMND Remote_OOB_Data_Request_Negative_Reply",
        0x0434 : "COMND IO_Capability_Request_Negative_Reply",
        0x0435 : "COMND Create_Physical_Link",
        0x0436 : "COMND Accept_Physical_Link",
        0x0437 : "COMND Disconnect_Physical_Link",
        0x0438 : "COMND Create_Logical_Link",
        0x0439 : "COMND Accept_Logical_Link",
        0x043a : "COMND Disconnect_Logical_Link",
        0x043b : "COMND Logical_Link_Cancel",
        0x043c : "COMND Flow_Spec_Modify",
        0x043d : "COMND Enhanced_Setup_Synchronous_Connection",
        0x043e : "COMND Enhanced_Accept_Synchronous_Connection_Request",
        0x043f : "COMND Truncated_Page",
        0x0440 : "COMND Truncated_Page_Cancel",
        0x0441 : "COMND Set_Connectionless_Slave_Broadcast",
        0x0442 : "COMND Set_Connectionless_Slave_Broadcast_Broadcast_Receive",
        0x0443 : "COMND Start_Synchronization_Train",
        0x0444 : "COMND Receive_Synchronization_Train",
        0x0445 : "COMND Remote_OOB_Extended_Data_Request_Reply",
        0x0801 : "COMND Hold_Mode",
        0x0803 : "COMND Sniff_Mode",
        0x0804 : "COMND Exit_Sniff_Mode",
        0x0805 : "COMND Park_State",
        0x0806 : "COMND Exit_Park_State",
        0x0807 : "COMND QoS_Setup",
        0x0809 : "COMND Role_Discovery",
        0x080b : "COMND Switch_Role",
        0x080c : "COMND Read_Link_Policy_Settings",
        0x080d : "COMND Write_Link_Policy_Settings",
        0x080e : "COMND Read_Default_Link_Policy_Settings",
        0x080f : "COMND Write_Default_Link_Policy_Settings",
        0x0810 : "COMND Flow_Specification",
        0x0811 : "COMND Sniff_Subrating",
        0x0c01 : "COMND Set_Event_Mask",
        0x0c03 : "COMND Reset",
        0x0c05 : "COMND Set_Event_Filter",
        0x0c08 : "COMND Flush",
        0x0c09 : "COMND Read_PIN_Type",
        0x0c0a : "COMND Write_PIN_Type",
        0x0c0b : "COMND Create_New_Unit_Key",
        0x0c0d : "COMND Read_Stored_Link_Key",
        0x0c11 : "COMND Write_Stored_Link_Key",
        0x0c12 : "COMND Delete_Stored_Link_Key",
        0x0c13 : "COMND Write_Local_Name",
        0x0c14 : "COMND Read_Local_Name",
        0x0c15 : "COMND Read_Connection_Accept_Timeout",
        0x0c16 : "COMND Write_Connection_Accept_Timeout",
        0x0c17 : "COMND Read_Page_Timeout",
        0x0c18 : "COMND Write_Page_Timeout",
        0x0c19 : "COMND Read_Scan_Enable",
        0x0c1a : "COMND Write_Scan_Enable",
        0x0c1b : "COMND Read_Page_Scan_Activity",
        0x0c1c : "COMND Write_Page_Scan_Activity",
        0x0c1d : "COMND Read_Inquiry_Scan_Activity",
        0x0c1e : "COMND Write_Inquiry_Scan_Activity",
        0x0c1f : "COMND Read_Authentication_Enable",
        0x0c20 : "COMND Write_Authentication_Enable",
        0x0c23 : "COMND Read_Class_of_Device",
        0x0c24 : "COMND Write_Class_of_Device",
        0x0c25 : "COMND Read_Voice_Setting",
        0x0c26 : "COMND Write_Voice_Setting",
        0x0c27 : "COMND Read_Automatic_Flush_Timeout",
        0x0c28 : "COMND Write_Automatic_Flush_Timeout",
        0x0c29 : "COMND Read_Num_Broadcast_Retransmissions",
        0x0c30 : "COMND Write_Num_Broadcast_Retransmissions",
        0x0c2b : "COMND Read_Hold_Mode_Activity",
        0x0c2c : "COMND Write_Hold_Mode_Activity",
        0x0c2d : "COMND Read_Transmit_Power_Level",
        0x0c2e : "COMND Read_Synchronous_Flow_Control_Enable",
        0x0c2f : "COMND Write_Synchronous_Flow_Control_Enable",
        0x0c31 : "COMND Set_Controller_To_Host_Flow_Control",
        0x0c33 : "COMND Host_Buffer_Size",
        0x0c35 : "COMND Host_Number_Of_Completed_Packets",
        0x0c36 : "COMND Read_Link_Supervision_Timeout",
        0x0c37 : "COMND Write_Link_Supervision_Timeout",
        0x0c38 : "COMND Read_Number_Of_Supported_IAC",
        0x0c39 : "COMND Read_Current_IAC_LAP",
        0x0c3a : "COMND Write_Current_IAC_LAP",
        0x0c3f : "COMND Set_AFH_Host_Channel_Classification",
        0x0c42 : "COMND Read_Inquiry_Scan_Type",
        0x0c43 : "COMND Write_Inquiry_Scan_Type",
        0x0c44 : "COMND Read_Inquiry_Mode",
        0x0c45 : "COMND Write_Inquiry_Mode",
        0x0c46 : "COMND Read_Page_Scan_Type",
        0x0c47 : "COMND Write_Page_Scan_Type",
        0x0c48 : "COMND Read_AFH_Channel_Assessment_Mode",
        0x0c49 : "COMND Write_AFH_Channel_Assessment_Mode",
        0x0c51 : "COMND Read_Extended_Inquiry_Response",
        0x0c52 : "COMND Write_Extended_Inquiry_Response",
        0x0c53 : "COMND Refresh_Encryption_Key",
        0x0c55 : "COMND Read_Simple_Pairing_Mode",
        0x0c56 : "COMND Write_Simple_Pairing_Mode",
        0x0c57 : "COMND Read_Local_OOB_Data",
        0x0c58 : "COMND Read_Inquiry_Response_Transmit_Power_Level",
        0x0c59 : "COMND Write_Inquiry_Response_Transmit_Power_Level",
        0x0c60 : "COMND Send_Key_Press_Notification",
        0x0c5a : "COMND Read_Default_Erroneous_Data_Reporting",
        0x0c5b : "COMND Write_Default_Erroneous_Data_Reporting",
        0x0c5f : "COMND Enhanced_Flush",
        0x0c61 : "COMND Read_Logical_Link_Accept_Timeout",
        0x0c62 : "COMND Write_Logical_Link_Accept_Timeout",
        0x0c63 : "COMND Set_Event_Mask_Page_2",
        0x0c64 : "COMND Read_Location_Data",
        0x0c65 : "COMND Write_Location_Data",
        0x0c66 : "COMND Read_Flow_Control_Mode",
        0x0c67 : "COMND Write_Flow_Control_Mode",
        0x0c68 : "COMND Read_Enhance_Transmit_Power_Level",
        0x0c69 : "COMND Read_Best_Effort_Flush_Timeout",
        0x0c6a : "COMND Write_Best_Effort_Flush_Timeout",
        0x0c6b : "COMND Short_Range_Mode",
        0x0c6c : "COMND Read_LE_Host_Support",
        0x0c6d : "COMND Write_LE_Host_Support",
        0x0c6e : "COMND Set_MWS_Channel_Parameters",
        0x0c6f : "COMND  Set_ External_Frame_Configuration",
        0x0c70 : "COMND Set_MWS_Signaling",
        0x0c71 : "COMND Set_MWS_Transport_Layer",
        0x0c72 : "COMND Set_MWS_Scan_Frequency_Table",
        0x0c73 : "COMND Set_MWS_PATTERN_Configuration",
        0x0c74 : "COMND Set_Reserved_LT_ADDR",
        0x0c75 : "COMND Delete_Reserved_LT_ADDR",
        0x0c76 : "COMND Set_Connectionless_Slave_Broadcast_Data",
        0x0c77 : "COMND Read_Synchronization_Train_Parameters",
        0x0c78 : "COMND Write_Synchronization_Train_Parameters",
        0x0c79 : "COMND Read_Secure_Connections_Host_Support",
        0x0c7a : "COMND Write_Secure_Connections_Host_Support",
        0x0c7b : "COMND Read_Authenticated_Payload_Timeout",
        0x0c7c : "COMND Write_Authenticated_Payload_Timeout",
        0x0c7d : "COMND Read_Local_OOB_Extended_Data",
        0x0c7e : "COMND Read_Extended_Page_Timeout",
        0x0c7f : "COMND Write_Extended_Page_Timeout",
        0x0c80 : "COMND Read_Extended_Inquiry_Length",
        0x0c81 : "COMND Write_Extended_Inquiry_Length",
        0x1001 : "COMND Read_Local_Version_Information",
        0x1002 : "COMND Read_Local_Supported_Commands",
        0x1003 : "COMND Read_Local_Supported_Features",
        0x1004 : "COMND Read_Local_Extended_Features",
        0x1005 : "COMND Read_Buffer_Size",
        0x1009 : "COMND Read_BD_ADDR",
        0x100a : "COMND Read_Data_Block_Size",
        0x100b : "COMND Read_Local_Supported_Codecs",
        0x1401 : "COMND Read_Failed_Contact_Counter",
        0x1402 : "COMND Reset_Failed_Contact_Counter",
        0x1403 : "COMND Read_Link_Quality",
        0x1405 : "COMND Read_RSSI",
        0x1406 : "COMND Read_AFH_Channel_Map",
        0x1407 : "COMND Read_Clock",
        0x1408 : "COMND Encryption_Key_Size",
        0x1409 : "COMND Read_Local_AMP_Info",
        0x140a : "COMND Read_Local_AMP_ASSOC",
        0x140b : "COMND Write_Remote_AMP_ASSOC",
        0x140c : "COMND Get_MWS_Transport_Layer_Configuration",
        0x140d : "COMND Set_Triggered_Clock_Capture",
        0x1801 : "COMND Read_Loopback_Mode",
        0x1802 : "COMND Write_Loopback_Mode",
        0x1803 : "COMND Enable_Device_Under_Test_Mode",
        0x1804 : "COMND Write_Simple_Pairing_Debug_Mode",
        0x1807 : "COMND Enable_AMP_Receiver_Reports",
        0x1808 : "COMND AMP_Test_End",
        0x1809 : "COMND AMP_Test",
        0x180a : "COMND Write_Secure_Connection_Test_Mode",
        0x2001 : "COMND LE_Set_Event_Mask",
        0x2002 : "COMND LE_Read_Buffer_Size",
        0x2003 : "COMND LE_Read_Local_Supported_Features",
        0x2005 : "COMND LE_Set_Random_Address",
        0x2006 : "COMND LE_Set_Advertising_Parameters",
        0x2007 : "COMND LE_Read_Advertising_Channel_Tx_Power",
        0x2008 : "COMND LE_Set_Advertising_Data",
        0x2009 : "COMND LE_Set_Scan_Responce_Data",
        0x200a : "COMND LE_Set_Advertise_Enable",
        0x200b : "COMND LE_Set_Set_Scan_Parameters",
        0x200c : "COMND LE_Set_Scan_Enable",
        0x200d : "COMND LE_Create_Connection",
        0x200e : "COMND LE_Create_Connection_Cancel ",
        0x200f : "COMND LE_Read_White_List_Size",
        0x2010 : "COMND LE_Clear_White_List",
        0x2011 : "COMND LE_Add_Device_To_White_List",
        0x2012 : "COMND LE_RemoveDevice_From_White_List",
        0x2013 : "COMND LE_Connection_Update",
        0x2014 : "COMND LE_Set_Host_Channel_Classification",
        0x2015 : "COMND LE_Read_Channel_Map",
        0x2016 : "COMND LE_Read_Remote_Used_Features",
        0x2017 : "COMND LE_Encrypt",
        0x2018 : "COMND LE_Rand",
        0x2019 : "COMND LE_Start_Encryption",
        0x201a : "COMND LE_Long_Term_Key_Request_Reply",
        0x201b : "COMND LE_Long_Term_Key_Request_Negative_Reply",
        0x201c : "COMND LE_Read_Supported_States",
        0x201d : "COMND LE_Receiver_Test",
        0x201e : "COMND LE_Transmitter_Test",
        0x201f : "COMND LE_Test_End",
        0x2020 : "COMND LE_Remote_Connection_Parameter_Request_Reply",
        0x2021 : "COMND LE_Remote_Connection_Parameter_Request_Negative_Reply",
        0xfc7e : "COMND VSC_Write_Dynamic_SCO_Routing_Change",
        0xfc57 : "COMND VSC_Write_High_Priority_Connection",
        0xfc4c : "COMND VSC_Write_RAM",
        0xfc4e : "COMND VSC_Launch_RAM",
        0xfc18 : "COMND VSC_Update_UART_Baud_Rate",
        0xfc01 : "COMND VSC_Write_BD_ADDR",
        0xfc1c : "COMND VSC_Write_SCO_PCM_Int_Param",
        0xfc27 : "COMND VSC_Set_Sleepmode_Param",
        0xfc1e : "COMND VSC_Write_PCM_Data_Format_Param",
        0xfc2e : "COMND VSC_Download_Minidriver",
        0xfd53 : "COMND VSC_BLE_VENDOR_CAP",
        0xfd54 : "COMND VSC_BLE_MULTI_ADV",
        0xfd56 : "COMND VSC_BLE_BATCH_SCAN",
        0xfd57 : "COMND VSC_BLE_ADV_FILTER",
        0xfd58 : "COMND VSC_BLE_TRACK_ADV",
        0xfd59 : "COMND VSC_BLE_ENERGY_INFO"
    }

def parse(data):
    """
    Parse HCI command

    References can be found here:
    * https://www.bluetooth.org/en-us/specification/adopted-specifications - Core specification 4.1
    ** [vol 2] Part E (Section 5) - HCI Data Formats
    ** [vol 2] Part E (Section 5.4) - Exchange of HCI-specific information

    All integer values are stored in "little-endian" order.

    Returns a tuple of (opcode, length, data)
    """
    opcode, length = struct.unpack("<HB", data[:3])
    data = data[3:]
    return opcode, length, data

def cmd_to_str(opcode):
    """
    Return a string representing the opcode
    """
    return HCI_COMMANDS[opcode]