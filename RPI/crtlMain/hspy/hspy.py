import ctypes
import os
import serial


def calCRC16(input_bytes):
	assert type(input_bytes) is bytes
	crcso = ctypes.cdll.LoadLibrary
	crclib = crcso("./libcrc16.so")
	crc_calc = crclib.CRC16(input_bytes,len(input_bytes))
	return crc_calc.to_bytes(2,byteorder='big')

def attached_CRC(input_bytes):
	crc_bytes=calCRC16(input_bytes)
	return input_bytes+crc_bytes

'''
error code:
0: No error
1: serial port is not found
2: address is not found or the clent has no response
3: send or CRC Error
'''
class hspy(object):
	def __init__(self, serial_port, addr,show_log=True):
		#self._device_addr= b'\x55'
		self._error=0
		self._U_set=0
		self._I_set=0
		self._cmd_write=b'\x10'
		self._cmd_read=b'\x03'
		self._addr_set_U=b'\x10\x00'
		self._addr_set_I=b'\x10\x01'
		self._addr_get_U=b'\x10\x02'
		self._addr_get_I=b'\x10\x03'
		self._addr_run_stop=b'\x10\x04'
		self._addr_rs_adder=b'\x10\x05'
		self._addr_key_lock=b'\x10\x06'
		self._data_len=b'\x00\x01'
		self._U_digits=1
		self._I_digits=1
		self._serial_port = serial_port
		self._show_log = show_log

		if not os.path.exists(serial_port):
			self._error=1
			print('Serial port:{} is not found!'.format(self._serial_port))
		else:
			'''
			self.serial = serial.Serial(self._serial_port, baudrate=9600,timeout=0.1)
			addr_code=addr.to_bytes(1, byteorder='big')
			msg = addr_code+self._cmd_read+self._addr_rs_adder+self._data_len
			msg_bytes=attached_CRC(msg)
			assert self.serial.write(msg_bytes) == len(msg_bytes)
			msg_back=self.serial.readline()
			if len(msg_back)==7 and msg_back[4:5] == addr_code:
				self._addr_code=addr_code
				print('HSPY power supply (Addr:{}) has been connected'.format(addr))
			else:
				self._error=2
				print('Address code: {} is not found!'.format(addr))
			'''
			# for com security, check addr skipped
			self.serial = serial.Serial(self._serial_port, baudrate=9600,timeout=0.1)
			addr_code=addr.to_bytes(1, byteorder='big')
			self._addr_code=addr_code
			print('Connected to HSPY power supply (Addr:{})'.format(addr))

	def _status_check(func):
		def wrapper(self,*args,**kwargs):
			result = None
			if self._error !=0:
				if self._show_log:
					print('Reinitializing the serial:{}'.format(self._serial_port))
				if not os.path.exists(self._serial_port):
					self._error=1
					if self._show_log:
						print('Serial port:{} is not found!'.format(self._serial_port))
				else:
					self.serial = serial.Serial(self._serial_port,baudrate=9600,timeout=0.1)
					msg=self._addr_code+self._cmd_read+self._addr_rs_adder+self._data_len
					msg_bytes=attached_CRC(msg)
					assert self.serial.write(msg_bytes) == len(msg_bytes)
					#msg_back=self.serial.readline()
					msg_back = self.serial.read(7)
					if len(msg_back)>0 and msg_back[4:5] == self._addr_code:
						self._error = 0
					else:	
						self._error = 2
						if self._show_log:
							print('Address code {} is not found!'.format(self._addr_code))
			if self._error ==0:
				try:
					result = func(self,*args,**kwargs) 
				except AssertionError as e:
					if self._show_log:
						print('AssertionError:{}'.format(str(e)))
					self._error = 3
				except Exception as e:
					if self._show_log:
						print('Error:{}'.format(str(e)))
			return result
		return wrapper

	def scan_addr(self,start=0,end=255):
		assert 0<=start<=end<=255, '0<=start<=end<=255 is not satisfied!'
		addr_result=[]
		for i in range(start,end+1):
			addr_code=i.to_bytes(1, byteorder='big')
			msg=addr_code+self._cmd_read+self._addr_rs_adder+self._data_len
			msg_bytes=attached_CRC(msg)
			assert self.serial.write(msg_bytes) ==len(msg_bytes)
			#msg_back=self.serial.readline()
			msg_back = self.serial.read(7)
			#assert len(msg_back) >0, 'message len is wrong'
			addr_return=msg_back[4:5]
			if addr_code == addr_return:
				addr_result.append(i)
		return addr_result

	def setUIdigits(self,U_digits=1,I_digits=1):
		self._U_digits = int(U_digits)
		self._I_digits = int(I_digits)

	@_status_check
	def setAddr(self, addr):
		msg=self._addr_code+self._cmd_write+self._addr_rs_adder+self._data_len
		addr_bytes=int(addr).to_bytes(2, byteorder='big')
		msg_bytes=attached_CRC(msg+b'\x02'+addr_bytes)
		assert self.serial.write(msg_bytes) == len(msg_bytes), 'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(8)
		assert len(msg_back) ==8, 'message len is wrong'
		assert msg_back == attached_CRC(msg), 'receive: crc error'

	@_status_check
	def setU(self, Uset):
		msg=self._addr_code+self._cmd_write+self._addr_set_U+self._data_len
		Uset_bytes=int(round(Uset*pow(10.0,self._U_digits))).to_bytes(2, byteorder='big')
		msg_bytes=attached_CRC(msg+b'\x02'+Uset_bytes)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(8)
		assert len(msg_back) ==8, 'message len is wrong'
		assert msg_back == attached_CRC(msg), 'receive: crc error'
	
	@_status_check
	def setI(self, Iset):
		msg=self._addr_code+self._cmd_write+self._addr_set_I+self._data_len
		#Unit is mA
		Iset_bytes=int(round(Iset*pow(10.0,self._I_digits))).to_bytes(2, byteorder='big')
		msg_bytes=attached_CRC(msg+b'\x02'+Iset_bytes)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(8)
		assert len(msg_back) ==8, 'message len is wrong'
		assert msg_back == attached_CRC(msg),'receive: crc error'
	
	@_status_check
	def run(self):
		msg=self._addr_code+self._cmd_write+self._addr_run_stop+self._data_len
		RSset_bytes=int(1).to_bytes(2, byteorder='big')
		msg_bytes=attached_CRC(msg+b'\x02'+RSset_bytes)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(8)
		assert len(msg_back) == 8, 'message len is wrong'
		assert msg_back == attached_CRC(msg),'receive: crc error'
	
	@_status_check
	def stop(self):
		msg=self._addr_code+self._cmd_write+self._addr_run_stop+self._data_len
		RSset_bytes=int(0).to_bytes(2, byteorder='big')
		msg_bytes=attached_CRC(msg+b'\x02'+RSset_bytes)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(8)
		assert len(msg_back) == 8, 'message len is wrong'
		assert msg_back == attached_CRC(msg),'receive: crc error'
	
	@_status_check
	def setKeyLock(self,lock=True):
		# need to check 1 , 0 meanning
		msg=self._addr_code+self._cmd_write+self._addr_key_lock+self._data_len
		if lock:
			set_bytes=int(1).to_bytes(2, byteorder='big')
		else:
			set_bytes=int(0).to_bytes(2, byteorder='big')
		msg_bytes=attached_CRC(msg+b'\x02'+set_bytes)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(8)
		assert len(msg_back) ==8, 'message len is wrong'
		assert msg_back == attached_CRC(msg),'receive: crc error'

	@_status_check
	def getKeyLock(self):
		msg=self._addr_code+self._cmd_read+self._addr_key_lock+self._data_len
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(7)
		assert len(msg_back) ==7 , 'message len is wrong'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		bytes_len = msg_back[2]
		assert bytes_len == 2,'receive: len error'
		self.IsLock=int.from_bytes(msg_back[3:5], byteorder='big')
		if self.IsLock==1:
			return True
		else:
			return False

	@_status_check
	def getAll(self):
		msg=self._addr_code+self._cmd_read+b'\x10\x00'+b'\x00\x07'
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(19)
		#print(msg_back)
		assert len(msg_back) == 19, 'message len is not 19'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		assert msg_back[2] == 14, 'receive: len error'
		self.Uset=int.from_bytes(msg_back[3:5], byteorder='big')/pow(10.0,self._U_digits)
		self.Iset=int.from_bytes(msg_back[5:7], byteorder='big')/pow(10.0,self._I_digits)
		self.Uout=int.from_bytes(msg_back[7:9], byteorder='big')/pow(10.0,self._U_digits)
		self.Iout=int.from_bytes(msg_back[9:11], byteorder='big')/pow(10.0,self._I_digits)
		self.IsRun = int.from_bytes(msg_back[11:13], byteorder='big')
		self.addr = int.from_bytes(msg_back[13:15], byteorder='big')
		self.IsLock = int.from_bytes(msg_back[15:17], byteorder='big')
		return {'uset':self.Uset,'iset':self.Iset,'uout':self.Uout,'iout':self.Iout,'run':self.IsRun,'addr':self.addr,'lock':self.IsLock}
		

	@_status_check
	def getIsRun(self):
		msg=self._addr_code+self._cmd_read+self._addr_run_stop+self._data_len
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(7)
		assert len(msg_back) ==7, 'message len is wrong'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		assert msg_back[2] == 2,'receive: len error'
		self.IsRun=int.from_bytes(msg_back[3:5], byteorder='big')
		return self.IsRun
	

	@_status_check
	def getUout(self):
		msg=self._addr_code+self._cmd_read+self._addr_get_U+self._data_len
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(7)
		assert len(msg_back) ==7, 'message len is wrong'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		bytes_len = msg_back[2]
		assert bytes_len == 2,'receive: len error'
		self.Uout=int.from_bytes(msg_back[3:5], byteorder='big')/pow(10.0,self._U_digits)
		return self.Uout
	
	

	@_status_check
	def getIout(self):
		msg=self._addr_code+self._cmd_read+self._addr_get_I+self._data_len
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(7)
		assert len(msg_back) ==7, 'message len is wrong'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		bytes_len = msg_back[2]
		assert bytes_len == 2,'receive: len error'
		self.Iout=int.from_bytes(msg_back[3:5], byteorder='big')/pow(10.0,self._I_digits)
		return self.Iout

	@_status_check
	def getUset(self):
		msg=self._addr_code+self._cmd_read+self._addr_set_U+self._data_len
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(7)
		assert len(msg_back) ==7, 'message len is wrong'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		bytes_len = msg_back[2]
		assert bytes_len == 2,'receive: len error'
		self.Uset=int.from_bytes(msg_back[3:5], byteorder='big')/pow(10.0,self._U_digits)
		return self.Uset

	@_status_check
	def getIset(self):
		msg=self._addr_code+self._cmd_read+self._addr_set_I+self._data_len
		msg_bytes = attached_CRC(msg)
		assert self.serial.write(msg_bytes) == len(msg_bytes),'send: len of bytes error'
		#msg_back = self.serial.readline()
		msg_back = self.serial.read(7)
		assert len(msg_back) ==7, 'message len is wrong'
		assert msg_back[-2:] == calCRC16(msg_back[:-2]),'receive: crc error'
		assert msg_back[0:1] == self._addr_code,'receive: addr error'
		assert msg_back[1:2] == self._cmd_read,'receive: cmd error'
		bytes_len = msg_back[2]
		assert bytes_len == 2,'receive: len error'
		self.Iset=int.from_bytes(msg_back[3:5], byteorder='big')/pow(10.0,self._I_digits)
		return self.Iset
	
