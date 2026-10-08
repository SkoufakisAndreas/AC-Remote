import serial 
import time 

# Initialize the commands
VALID_COMMANDS = [
    "OFF", "ON", "TEMPUP", "TEMPDOWN", 
    "MODEAUTO", "MODECOOL", "MODEDRY", "MODEHEAT", "MODEFAN",
    "FANAUTO", "FAN1", "FAN2", "FAN3", "FAN4", "FAN5", "PURE", "NOPURE"
]

# Create the object 
board = serial.Serial('COM4', 115200, timeout=2)

print("***REMOTE TEST***")
print('Only use the orders below (Upper case or lowercase, doesn\'t matter) leaving NO SPACES:')

for cmd in VALID_COMMANDS:
    print(f"-{cmd}")

print('-SETTEMP')

   

# SOFTWARE RESET THE BOARD
board.write(b'R')

# Wait to receive the initialize message
response = board.readline()
print(response.decode('utf-8'))

def main():

    on_first = 0;

    while True:
        function = input("\nType a command (or EXIT): ").strip().upper()

        if function == 'EXIT':
            print("Exiting program...")
            break

        if function == 'ON':
            on_first = 1; 

       
            
        if function in VALID_COMMANDS:
            if on_first == 0:
                print("Type ON before any other command!\r\n")
                continue 

            else:
             if function == 'OFF':
                 on_first = 0;
             idx = VALID_COMMANDS.index(function)
             data_to_send = bytes([idx + 65])
             board.write(data_to_send)

             #Wait for the board to send the signal
             time.sleep(1)

             #Receive acknoledge information
             ack = board.readline().strip()
             #Convert to ascii decimal value 
             ack = ack.decode()
             ack = ord(ack)
             ack = ack - 65
             print(f'Command {VALID_COMMANDS[ack]} was executed...')

        elif function == 'SETTEMP':
             #Pre-defined letter for SETTEMP    
             board.write(b'S')
             temp = input("Enter the temp (17 - 30): ")
             #Make temp a digit 
             temp = int(temp)
             if temp > 30 :
                 temp = 30
             if temp < 17:
                 temp = 17
            #Use bytes to send temp numbers as bytes 
             board.write(bytes([temp]))    
             print(f"Set temperature to: {temp}°C")     

        else:
            print("Incorrect Command!!")


main()