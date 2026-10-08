import serial 

nucleo_board = serial.Serial('COM3', 115200)


def main():

    print("***IR Decode Programm***\n")
    print("To exit type 'exit' and press ENTER.\n")
    
    nucleo_board.write(b'R')
    
    #Once to read the initialization message that the board transmits 
    response = nucleo_board.readline()

    print(response.decode()) 

    while True:
       
       #Receive the function choice and add strip to by pass gaps etc
       user_input = input("Write the function that you want and then press the matching button on the remote : ").strip()
        #Use lower in case user has brain damage :)
       if user_input.lower() == 'exit':
            break
       
        #Safety to ignore by mistake ready
       if not user_input:
            continue
       
       user_input = user_input.upper()
       response = nucleo_board.readline()
       coded_msg =response.decode()
  
       decode_ir(coded_msg, user_input)

def decode_ir (raw_string, function):

    #First split the message:
    
    data_list = raw_string.split()

    #make the pulses array where all the data are turned into integers
    pulses = [int(x) for x in data_list]

    #Initialize the binary_code 
    binary_code = ""


    #remote sends the same signal twice. The sequence for repetition is 7421-4400-4301
    #sometimes (maybe because of noise) we receive 4300-4300-gap and then the message 
    #while others 4300-gap and then the message 

    start_value = 0;
    
    while pulses[start_value] > 2000:
       start_value += 1


    for i in range(start_value + 1, len(pulses), 2):
    # Even though we increment 2 each time the repeat pulses are 7420-4400-4300 so we will fall into one of them
      if pulses[i] > 2000:
        break  # Break stop the loop we dont need the repeat
    
      if pulses[i] > 1000:
        binary_code += "1"  # If gap huge its one
      else:
        binary_code += "0"  # if gap small its zero

    #create spaced binary 
    spaced_binary = " ".join(binary_code[i:i+8] for i in range(0, len(binary_code), 8))

    #Create result message        
    left_part = f"Bits for order {function} are :"
    result_text = f"{left_part:<60} {spaced_binary}"

    #Print it on terminal just to see its working
    print(result_text)
    
    #open file - append not overwrite- and utf-8 encoding 
    with open("my_ir_signals.txt", "a", encoding="utf-8") as file:
        file.write(result_text + "\n")
        #Write the result_text and add newline


main()