#Meeprommer commandline interface
#By Zack Nelson
#Project Home:
#https://github.com/mkeller0815/MEEPROMMER
#http://www.ichbinzustaendig.de/dev/meeprommer-en

#Adapted to work with EEPROMDate programmer
#By Svetlana Tovarisch

import serial, sys, argparse, time

# Parse command line arguments
parser = argparse.ArgumentParser(
        description='Meepromer Command Line Interface',
        epilog='Read source for further information')

task = parser.add_mutually_exclusive_group()
task.add_argument('-w', '--write', dest="cmd", action="store_const",
        const="write", help='Write to EEPROM')
task.add_argument('-W', '--write_paged', dest="cmd", action="store_const",
        const="write_paged", help='Fast paged write to EEPROM')
task.add_argument('-r', '--read', dest="cmd", action="store_const",
        const="read", help='Read from EEPROM as ascii')
task.add_argument('-d', '--dump', dest="cmd", action="store_const",
        const="dump", help='Dump EEPROM to binary file')
task.add_argument('-v', '--verify', dest="cmd", action="store_const",
        const="verify", help='Compare EEPROM with file')
task.add_argument('-V', '--version', dest="cmd", action="store_const",
        const="version", help='Check burner version')
task.add_argument('-u', '--unlock', dest="cmd", action="store_const",
        const="unlock", help='Unlock EEPROM')
task.add_argument('-l', '--list', dest="cmd", action="store_const",
        const="list", help='List serial ports')
task.add_argument('-D', '--debug', dest="cmd", action="store_const",
        const="debug", help='run debug code')

parser.add_argument('-a', '--address', action='store', default='0',
        help='Starting eeprom address (as hex), default 0')
parser.add_argument('-o', '--offset', action='store', default='0',
        help='Input file offset (as hex), default 0')
parser.add_argument('-b', '--bytes', action='store', default='512',
        type=long, help='Number of kBytes to r/w, default 8')
parser.add_argument('-p', '--page_size', action='store', default='256',
        type=long, help='Number of bytes per EEPROM page e.g.:'+
            'CAT28C*=32, AT28C*=64, X28C*=64, default 32')
parser.add_argument('-f', '--file', action='store',
        help='Name of data file')
parser.add_argument('-c', '--com', action='store', 
        default='COM3', help='Com port address')
parser.add_argument('-s', '--speed', action='store', 
        type=int, default='460800', help='Com port baud, default 460800')

def list_ports():
    from serial.tools import list_ports
    for x in list_ports.comports():
        print(x[0], x[1])

def dump_file():
    ser.flushInput()
    ser.write(bytes("B "+format(args.address,'08x')+" "+
        format(args.bytes*1024,'08x')+" 10\n").encode('ascii'))
    print(bytes("B "+format(args.address,'08x')+" "+
        format(args.bytes*1024,'08x')+" 10\n").encode('ascii'))
    eeprom = ser.read(args.bytes*1024)
    if(ser.read(1) != b'\0'):
        print("Error: no Ack")
        #sys.exit(1)
    try:
        fo = open(args.file,'wb+')
    except OSError:
        print("Error: File cannot be opened, verify it is not in use")
        sys.exit(1)
    fo.write(eeprom)
    fo.close()

def verify():
    print("Verifying...")
    ser.flushInput()
    ser.write(bytes("B "+format(args.address,'08x')+" "+
        format(args.bytes*1024,'08x')+" 10\n").encode('ascii'))
    try:
        fi = open(args.file,'rb')
    except FileNotFoundError:
        print("Error: ",args.file," not found, please select a valid file")
        sys.exit(1)
    except TypeError:
        print("Error: No file specified")
        sys.exit(1)
        
    fi.seek(args.offset)
    file = fi.read(args.bytes*1024)
    eeprom = ser.read(args.bytes*1024)
    if ser.read(1) != b'\0':
        print("Error: no EOF received")
        
    if file != eeprom:
        print("Not equal")
        n = 0
        for i in range(args.bytes*1024):
            if file[i] != eeprom[i]:
                n+=1
        print(n,"differences found")
        sys.exit(1)
    else:
        print("Ok")
        sys.exit(0)
    if(ser.read(1) != b'\0'):
        print("Error: no Ack")
        sys.exit(1)

def read_eeprom():
    ser.flushInput()
    ser.write(bytes("R "+format(args.address,'08x')+" "+
        format(args.address+args.bytes*1024,'08x')+
        " 10\n").encode('ascii'))
    ser.readline()#remove blank starting line
    for i in range(int(round(args.bytes*1024/16))):
        print(ser.readline().decode('ascii').rstrip())

def write_eeprom(paged):
    import time
    
    fi = open(args.file,'rb')
    fi.seek(args.offset)
    now = time.time() #start our stopwatch
    for i in range(args.bytes*4): #write n blocks of 256 bytes
        #if(i % 128 == 0):
        #    print("Block separation")
        #    time.sleep(1)
        output = fi.read(256)
        print("Writing from",format(args.address+i*256,'08x'),
              "to",format(args.address+i*256+255,'08x'))
        if paged:
            ser.write(bytes("W "+format(args.address+i*256,'08x')+
                " 00000100 "+format(args.page_size,'02x')+"\n").encode('ascii'))
        else:
            ser.write(bytes("W "+format(args.address+i*256,'08x')+
                " 00000100 00\n").encode('ascii'))

        ser.flushInput()
        ser.write(output)
        #time.sleep(0.08)
        #if(ser.read(1) != b'%'):
        #    print("Error: no Ack")
        #    sys.exit(1)
        while(ser.read(1) != b'%'):
            time.sleep(0.01)
    print("Wrote",args.bytes*1024,"bytes in","%.2f"%(time.time()-now),"seconds")
    
def unlock():
    print("Unlocking...")
    ser.flushInput()
    ser.write(bytes("U 00000000 00000000 00\n").encode('ascii'))
    if ser.read(1) != b'%':
        print("Error: no ack")
        sys.exit(1)

def version():
    print("Burner version:")
    ser.flushInput()
    ser.write(bytes("V 00000000 00000000 00\n").encode('ascii'))
    if ser.read(1) != b'E':
        print("Error: no ack")
        sys.exit(1)
    print(ser.read(5))
    
    
args = parser.parse_args()
#convert our hex strings to ints
args.address = long(args.address,16)
args.offset = long(args.offset,16)

SERIAL_TIMEOUT = 1200 #seconds
try:
    ser = serial.Serial(args.com, args.speed, timeout=SERIAL_TIMEOUT)
    time.sleep(2)
except serial.serialutil.SerialException:
    print("Error: Serial port is not valid, please select a valid port")
    sys.exit(1)

if args.cmd == 'write':
    write_eeprom(False)
elif args.cmd == 'write_paged':
    write_eeprom(True)
    #verify()
elif args.cmd == 'read':
    read_eeprom()
elif args.cmd == 'dump':
    dump_file()
elif args.cmd == 'verify':
    verify()
elif args.cmd == 'unlock':
    unlock();
elif args.cmd == 'list':
    list_ports()
elif args.cmd == 'version':
    version()
elif args.cmd == 'debug':
    """args.bytes = 32
    args.file = 'pb.bin4'
    args.address = 98304
    write_eeprom(True)
    verify()
    args.file = 'pb.bin3'
    args.address = 65536
    write_eeprom(True)
    verify()
    args.file = 'pb.bin2'
    args.address = 32768
    write_eeprom(True)
    verify()
    args.file = 'pb.bin1'
    args.address = 0
    write_eeprom(True)
    verify()
    args.bytes = 128
    args.file = 'pb.bin'
    verify()"""
    args.bytes = 32
    args.file = 'chip.bin4'
    args.address = 98304
    dump_file()
    args.file = 'chip.bin3'
    args.address = 65536
    dump_file()
    args.file = 'chip.bin2'
    args.address = 32768
    dump_file()
    args.file = 'chip.bin1'
    args.address = 0
    dump_file()
    args.bytes = 128
    args.file = 'chip.bin'
    dump_file()

ser.close()
sys.exit(0)
