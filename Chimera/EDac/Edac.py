def getHex(decimal):
    hexString = hex(decimal)
    hexString = '0'+hexString[2:]
    return hexString

def getDec(voltage):
    num = (voltage+10)
    dec_max = 1048575
    decimal = dec_max/20*num
    return int(decimal)

def get_message(volt_list):
    message = ''
    for volt in volt_list:
        dec = getDec(volt)
        hex = getHex(dec)
        message = message+hex
    return message