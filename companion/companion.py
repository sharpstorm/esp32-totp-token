import serial

class CompanionApp:
  def __init__(self, ser):
    self.serial = ser

  def sendCommand(self, commandCode):
    self.serial.write(bytes([0xDE, commandCode, 0xA0]))

  def handleShowDebug(self):
    self.sendCommand(1)

  def handleShowSecret(self):
    index = input('Show Secret Idx: ')
    try:
      index = int(index)
    except:
      print('Invalid index')
      return
    
    self.sendCommand(2)
    self.serial.write(bytes([index]))

  def handleDeleteSecret(self):
    index = input('Delete Secret Idx: ')
    try:
      index = int(index)
    except:
      print('Invalid index')
      return
    
    self.sendCommand(3)
    self.serial.write(bytes([index]))

  def handlePutSecret(self):
    name = input('Enter Name: ')
    secret = input('Enter Secret: ')

    print(bytes(name, 'ascii'))
    
    self.sendCommand(4)
    self.serial.write(bytes([len(secret), len(name)]))
    self.serial.write(bytes(secret, 'ascii'))
    self.serial.write(bytes(name, 'ascii'))

  def handleReadSerial(self):
    data = self.serial.read(self.serial.in_waiting)
    print(str(data, encoding='ascii'))

  def printCommandList(self):
    print('1. Show Debug')
    print('2. Show Secret')
    print('3. Delete Secret')
    print('4. Put Secret')
    print('5. Drain Serial')

  def start(self):
    while True:
      self.printCommandList()
      command = input('Enter Command: ')

      if command == '1':
        self.handleShowDebug()
      elif command == '2':
        self.handleShowSecret()
      elif command == '3':
        self.handleDeleteSecret()
      elif command == '4':
        self.handlePutSecret()
      elif command == '5':
        self.handleReadSerial()
      elif command == 'exit':
        break
      print()
      


print('TOTP Token Companion Tool')
with serial.Serial('COM6', 115200, timeout=2) as ser:
  print('Connection Established')

  app = CompanionApp(ser)
  app.start()
