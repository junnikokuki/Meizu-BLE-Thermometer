import os
from btsnoop.android.snoopphone import SnoopPhone
import btsnoop.btsnoop.btsnoop as btsnoop
import btsnoop.bt.hci_uart as hci_uart
import btsnoop.bt.hci_acl as hci_acl
import btsnoop.bt.l2cap as l2cap
import btsnoop.bt.att as att
import argparse

def get_ir_infos(records):
    ir_infos = {}
    for record in records:
        seq_nbr = record[0]

        hci_pkt_type, hci_pkt_data = hci_uart.parse(record[4])

        if hci_pkt_type != hci_uart.ACL_DATA:
            continue

        hci_data = hci_acl.parse(hci_pkt_data)
        l2cap_length, l2cap_cid, l2cap_data = l2cap.parse(hci_data[2], hci_data[4])

        if l2cap_cid != l2cap.L2CAP_CID_ATT:
            continue

        att_opcode, att_data = att.parse(l2cap_data)

        data = att_data

        if att_opcode == 0x12:
            if len(data) > 6:
                if data[2] == 0x55 and data[5] == 0x00: #real ir data
                    ir_seq = int(data[4])
                    send_seq = int(data[6])
                    if send_seq == 0:
                        packet_count = int(data[7])
                        ir_id = data[8:].hex()
                        ir_data_list = []
                        index = 1
                        while index < packet_count:
                            ir_data_list.append('')
                            index = index + 1
                        ir_info = {'id':ir_id, 'data':ir_data_list }
                        ir_infos[ir_seq] = ir_info
                    else:
                        ir_info = ir_infos[ir_seq]
                        if send_seq <= len(ir_info['data']):
                            ir_data = data[7:].hex()
                            ir_data_list = ir_info['data']
                            ir_data_list[send_seq - 1] = ir_data
                            ir_info['data'] = ir_data_list
                            ir_infos[ir_seq] = ir_info
        else:
            continue
        
    sorted_keys = sorted(ir_infos.keys())
    for k in sorted_keys:
        ir_info = ir_infos[k]
        print(ir_info['id'] + ':' + ''.join(ir_info['data']))
        print(' ')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=(
            "Extract IR data from android phone or btsnoop_hci.log file"))
    parser.add_argument("-f", type=str, dest='file', default=None, help="log file path, if not specified, pull it from phone with adb.")
    args = parser.parse_args()
    
    filename = ''
    if args.file != None:
        filename = os.path.expanduser(args.file)
        if not os.path.isfile(filename):
            print("Invalid log file path specified!")
            sys.exit(-1)
    else:
        phone = SnoopPhone()
        home = os.path.expanduser("~")
        dst = os.path.join(home, 'mysnoop.log')
        filename = phone.pull_btsnoop(dst)
    
    records = btsnoop.parse(filename)
    get_ir_infos(records)