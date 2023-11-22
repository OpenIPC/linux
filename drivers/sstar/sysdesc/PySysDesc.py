# import module
import os
import sys
import copy
import struct
import math

class HeaderParser():
	def __init__(self):
		self.ImportString = ""
		self.dict = {}
		self.DefineName = []
		self.DefineValue = []

	def ProcessComment(self, SourceList): 
		State = 0
		Flag = 0
		FilterOut = ""
		for Line in SourceList:
			for i in range(len(Line)):
				Char = Line[i]
				if State == 0:
					if Char == '/':
						State = 1
					else:
						FilterOut += Char
						if Char == '\'':
							State = 6
						elif Char == '\"':
							State = 8
					continue
				if State == 1:
					if Char == '*':
						State = 3
					elif Char == '/':
						State = 4
					else:
						FilterOut += '/'
						FilterOut += Char
						State = 0
					continue
				if State == 2:
					if Char == '*':
						Flag = 1
						State = 3
					else:
						Flag = 0
						State = 2
					continue
				if State == 3:
					if Char == '/':
						if Flag == 1:
							FilterOut += ' '
						State = 0
					elif Char == '*':
						Flag = 1
						State = 3
					else:
						Flag = 0
						State = 2
					continue
				if State == 4:
					if Char == '\\':
						State = 5
					elif Char == '\n':
						FilterOut += "\r\n"
						State = 0
					else:
						State = 4
					continue
				if State == 5:
					if Char == '\\' or Char == '\r' or Char == '\n':
						if Char == '\n':
							FilterOut += "\r\n"
						State = 5
					else:
						State = 4
					continue
				if State == 6:
					FilterOut += Char
					if Char == '\\':
						State = 7
					elif Char == '\'':
						State = 0
					else:
						State = 6
					continue
				if State == 7:
					FilterOut += Char
					State = 6
					continue
				if State == 8:
					FilterOut += Char
					if Char == '\\':
						State = 9
					elif Char == '\"':
						State = 0
					else:
						State = 8
					continue
				if State == 9:
					FilterOut += Char
					State = 8
					continue
		return FilterOut

	def ProcessIfdef(self, List):
		WaitList = List
		Stack = []
		self.dict = {}
		for Tmp in WaitList:
			Keywords = Tmp[0]
			Name = Tmp[1]
			Value = Tmp[2]
			if Keywords == 'ifdef':
				if Stack == [] or Stack[-1] == 'light':
					if self.dict.get(Name) != None:
						Stack.append('light')
					else:
						Stack.append('dark')
				elif Stack[-1] == 'dark' or Stack[-1] == 'inComment':
					Stack.append('inComment')
			elif Keywords == 'ifndef':
				if Stack == [] or Stack[-1] == 'light':
					if self.dict.get(Name) == None:
						Stack.append('light')
					else:
						Stack.append('dark')
				elif Stack[-1] == 'dark' or Stack[-1] == 'inComment':
					Stack.append('inComment')
			elif Keywords == 'define':
				if Stack == [] or Stack[-1] == 'light':
					self.dict[Name] = Value
					if Value != 'None':
						self.DefineName.append(Name)
						self.DefineValue.append(Value)
			elif Keywords == 'endif':
				if Stack != []:
					del Stack[-1]
			elif Keywords == 'else':
				if Stack == []:
					raise Exception('empty stack')
				if Stack[-1] == 'light':
					Stack[-1] = 'dark'
				elif Stack[-1] == 'dark':
					Stack[-1] = 'light'
			elif Keywords == 'undef':
				if Stack == [] or Stack[-1] == 'light':
					del self.dict[Name]

	def ProcessTab(self,Str):
		StrTmp = ''
		Stack = []
		for i in Str:  
			if Stack == []:
				if i == '\'' or i == '\"':
					Stack.append(i)
			else:
				if i == '\'' or i == '\"':
					if Stack[0] == i:
						Stack = []
					else:
						Stack.append(i)
			if i == '\t' and Stack == []:
				StrTmp += ' '
			StrTmp += i
		return StrTmp

	def ProcessString(self,String):
		Lines = String.split('\n')
		List = []
		for Str in Lines:
			Str = self.ProcessTab(Str)
			Str = Str.strip()
			if Str != '' and Str[0] == '#':
				Str = Str[1:]
				Str = Str.strip()
				Tl = Str.split(' ', 1)
				Keywords = Tl[0]
				if Keywords == 'else' or Keywords == 'endif':
					List.append([Keywords, 'None', 'None'])
				elif Keywords == 'ifdef' or Keywords == 'ifndef' or Keywords == 'undef':
					Ns = Tl[1].strip()
					Nl = Ns.split(' ', 1)
					List.append([Keywords, Nl[0], 'None'])
				elif Keywords == 'define':
					Ns = Tl[1].strip()
					Nl = Ns.split(' ', 1)
					if len(Nl) == 2:
						List.append([Keywords, Nl[0], Nl[1].strip()])
					elif len(Nl) == 1:
						List.append([Keywords, Nl[0], 'None'])
		return List

	def Str2Num(self,String,FrontPos,RearPos,index):
		num = -1
		TmpValue = String[FrontPos:RearPos]
		TmpValue = TmpValue.strip()
		if TmpValue.isdigit() == True:
			num = int(TmpValue,10)
		elif TmpValue[0:1] == '0x' or TmpValue[0:1] == '0X':
			num = int(TmpValue,16)
		else:
			for k in range(0,index,1):
				if self.DefineName[k] == TmpValue:
					TmpValue = self.DefineValue[k]
					if TmpValue.isdigit() == True:
						num = int(TmpValue,10)
						break
					elif TmpValue[0:2] == '0x' or TmpValue[0:2] == '0X':
						num = int(TmpValue,16)
						break
					else:
						print "!!! syntax error in define value " + TmpValue
		return num

	def ProcessDefineValue(self):
		OperatorList = []
		OperatorPos = []
		NumPosList = []
		NumList = []
		Operator = ''
		Ans = 0
		flag = 0
		for i in range(0,len(self.DefineValue),1):
			tmp = self.DefineValue[i]
			if tmp.isdigit() == True:
				continue
			elif tmp[0:2] == '0x' or tmp[0:2] == '0X':
				continue
			else:
				for j in range(0,len(tmp),1):   # scan operator
					if ((tmp[j] == '+') or (tmp[j] == '-') or (tmp[j] == '*') or (tmp[j] == '/') or (tmp[j] == '%')
						or (tmp[j] == '&') or (tmp[j] == '|') or (tmp[j] == '^') or (tmp[j] == '~') or (tmp[j] == '<') or (tmp[j] == '>')
						or (tmp[j] == '(') or (tmp[j] == ')')):
						OperatorList.append(tmp[j])
						OperatorPos.append(j)
				if len(OperatorList) == 0:   # without expression , replace string 
					for j in range(0,i,1):
						if self.DefineName[j] == self.DefineValue[i]:
							self.DefineValue[i] = self.DefineValue[j]   # update self.DefineValue[i] 
							if self.dict.get(self.DefineName[i]) != None:
								self.dict[self.DefineName[i]] = self.DefineValue[i]   # update self.dict.values
								break
							else:
								print "!!! syntex error : Can't find out the define name !!!"
								return False
				else:  # with expression
					for j in range(0,len(OperatorList),1):   # get define value position
						if OperatorList[j] == '(':
							NumPosList.append(OperatorPos[j]+1)
						elif OperatorList[j] == ')':
							NumPosList.append(OperatorPos[j])
						else:
							NumPosList.append(OperatorPos[j])
							if (OperatorList[j] == '<') or (OperatorList[j]  == '>'):
								NumPosList.append(OperatorPos[j]+2)
							else:
								NumPosList.append(OperatorPos[j]+1)
					for j in range(0,len(NumPosList), 1):   # define value convert into number
						if flag == 0:
							flag = 1;
						else:
							num = self.Str2Num(tmp,NumPosList[j-1],NumPosList[j],i)
							NumList.append(num)
							flag = 0;
					flag = 0
					for j in range(0,len(OperatorList),1):  # Accumulate calculation
						Operator = OperatorList[j]
						if Operator == '(':
							continue
						elif Operator == ')':
							self.DefineValue[i] = str(Ans)   # update self.DefineValue[i]
							if self.dict.get(self.DefineName[i]) != None:
								self.dict[self.DefineName[i]] = self.DefineValue[i]   # update self.dict.values
							else:
								print "!!! syntex error : Can't find out the define name !!!"
								return False
							OperatorList = []
							OperatorPos = []
							NumPosList = []
							NumList = []
							Ans = 0
							flag = 0
						else:
							if flag == 0:
								num = NumList[flag]
								num2 = NumList[flag+1]
								flag = flag + 1
							else:
								num = Ans
								num2 = NumList[flag+1]
								flag = flag + 1
							if (num >= 0) and (num2 >= 0):
								if Operator == '+':
									Ans = num + num2
								elif Operator == '-':
									Ans = num - num2
								elif Operator == '*':
									Ans = num * num2
								elif Operator == '/':
									Ans = num / num2
								elif Operator == '%':
									Ans = num % num2
								elif Operator == '&':
									Ans = num & num2
								elif Operator == '|':
									Ans = num | num2
								elif Operator == '^':
									Ans = num ^ num2
								elif Operator == '<':
									Ans = num << num2
								elif Operator == '>':
									Ans = num >> num2
								else:
									print "!!! syntex error : no support operator in " + Operator + "!!!"
									return False
							else:
								print "!!! syntex error : the define value is error in " + tmp + "!!!"
								return False
		self.DefineName = []
		self.DefineValue = []
		return True

	def LoadFile(self,FileName):
		try:
			ReadFd = open(FileName, 'r')
		except IOError:
			print 'load header file error'
			return False
		else:
			ResultList = []
			for Line in ReadFd:
				Line = Line.lstrip()
				if len(Line):
					ResultList.append(Line)
			ReadFd.close()
			#print ResultList
			self.ImportString = self.ProcessComment(ResultList) + '\r\n'  # filter out comment 
			#print self.ImportString
			ResultList = []
			ResultList = self.ProcessString(self.ImportString)  #ResultList[0] = keywords ; ResultList[1] = name ; ResultList[2] = value
			#print ResultList
			self.ProcessIfdef(ResultList)   # filter out #ifndef ....
			#print self.DefineName
			#print self.DefineValue
			#print self.dict
			RetureVal = self.ProcessDefineValue()
			#print self.dict
			return RetureVal

	def string2other(self,s):
		s = s.strip()
		if s == '':
			return s
		elif s == 'false':  # bool
			return False
		elif s == 'true':
			return True
		elif s == 'None':
			return None
		elif s[0] == '\'' and s[-1] == '\'':
			s = s[1:-1]
			hexlist = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'A', 'B', 'C','D', 'E', 'F']
			octallist = ['0', '1', '2', '3', '4', '5', '6', '7']
			escape_dict = {'\\': '\\', '\'': '\'', '\"': '\"', 'a': '\a', 'b': '\b', 'n': '\n', 'v': '\v', 't': '\t','r': '\r','f':'\f'}
			if len(s) > 3:
				if s[-4] == '\\':
					if s[-3] == 'x':
						if s[-1] in hexlist and s[-2] in hexlist:
							return int('0'+ s[-3:], 16)
					elif s[-3] in octallist and s[-2] in octallist and s[-1] in octallist:
						return int(s[-3:],8)
			if len(s) >2:
				if s[-3] == '\\':
					if s[-2] in octallist and s[-1] in octallist:
						return int(s[-2:],8)
			if len(s) >1:
				if s[-2] == '\\':
					if s[-1] in octallist:
						return int(s[-1],8)
					if s[-1] in escape_dict:
						return ord(escape_dict[s[-1]])
			if len(s) >0:
				return ord(s[-1])


		elif s[0] == '\"' and s[-1] == '\"':
			s = s[1:-1]
			flag = 1
			stringTmp = ''
			for i in range(len(s)):
				if i < len(s) - 1:
					if s[i] == '\\' and s[i+1] == '\"':
						stringTmp += '\\'
						continue
				if flag == 1 and s[i] != '\"':
					stringTmp += s[i]
				if s[i] == '\"':
					if i > 0:
						if s[i - 1] == '\\':
							stringTmp += s[i]
						else:
							flag *= -1
					elif i == 0:
						flag *= -1
			tt = ''
			#escape = ['\\','\'','\"','a','b','n','v','t','r']
			escape_dict = {'\\':'\\','\'': '\'','\"':'\"','a':'\a','b':'\b','n':'\n','v':'\v','t':'\t','r':'\r','f':'\f'}
			hexlist = ['0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f','A','B','C','D','E','F']
			octallist = ['0','1','2','3','4','5','6','7']
			k = 0
			while(k < len(stringTmp)):
				if stringTmp[k] == '\\':
					if k < len(stringTmp) - 1:
						if  k < len(stringTmp) -2:
							if k < len(stringTmp) - 3:
								if stringTmp[k+1] in octallist and stringTmp[k+2] in octallist and stringTmp[k+3] in octallist:
									ss = stringTmp[k+1]+stringTmp[k+2]+stringTmp[k+3]
									tt += chr(int(ss,8))
									k += 4
									continue
								elif stringTmp[k + 1] == 'x':
									if stringTmp[k+2] in hexlist and stringTmp[k+3] in hexlist:
										ss = '0' + stringTmp[k+1]+stringTmp[k+2]+stringTmp[k+3]
										tt += chr(int(ss,16))
										k += 4
										continue
							elif stringTmp[k + 1] in octallist and stringTmp[k + 2] in octallist:
								ss = stringTmp[k + 1] + stringTmp[k + 2]
								tt += chr(int(ss, 8))
								k += 3
								continue
							elif stringTmp[k+1] == 'x' and stringTmp[k + 2] in hexlist:
								ss = '0' + stringTmp[k+1] + stringTmp[k+2]
								tt += chr(int(ss,16))
								k += 4
								continue
						if stringTmp[k + 1] in octallist:
							ss = stringTmp[k + 1]
							tt += chr(int(ss, 8))
							k += 2
							continue
						if stringTmp[k+1] in escape_dict:
							tt += escape_dict[stringTmp[k+1]]
						else:
							tt += stringTmp[k + 1]
						k += 2
						continue
				else:
					tt += stringTmp[k]
					k += 1
			return tt
		elif s[0] == 'L' :
			s = s[2:-1]
			flag = 1
			stringTmp = ''
			for i in range(len(s)):
				if i < len(s) - 1:
					if s[i] == '\\' and s[i+1] == '\"':
						stringTmp += '\\'
						continue
				if flag == 1 and s[i] != '\"':
					stringTmp += s[i]
				if s[i] == '\"':
					if i > 0:
						if s[i - 1] == '\\':
							stringTmp += s[i]
						else:
							flag *= -1
					elif i == 0:
						flag *= -1
			tt = ''
			#escape = ['\\','\'','\"','a','b','n','v','t','r']
			escape_dict = {'\\':'\\','\'': '\'','\"':'\"','a':'\a','b':'\b','n':'\n','v':'\v','t':'\t','r':'\r','f':'\f'}
			k = 0
			while(k < len(stringTmp)):
				if stringTmp[k] == '\\':
					if k < len(stringTmp) - 1:
						if k < len(stringTmp) - 3:
							if stringTmp[k+1:k+4].isdigit() == True:
								ss = stringTmp[k+1]+stringTmp[k+2]+stringTmp[k+3]
								tt += chr(int(ss,8))
								k += 4
								continue
							elif stringTmp[k + 1] == 'x':
								ss = '0' + stringTmp[k+1]+stringTmp[k+2]+stringTmp[k+3]
								tt += chr(int(ss,16))
								k += 4
								continue
						if stringTmp[k+1] in escape_dict:
							tt += escape_dict[stringTmp[k+1]]
						else:
							tt += stringTmp[k + 1]
						k += 2
						continue
				else:
					tt += stringTmp[k]
					k += 1
			return unicode(tt)
		elif s[0] == '{' :
			if s[-1] == ';':
				s = s[1:-2]
			else:
				s = s[1:-1]
			mysplit = []
			mark = 1
			stack = []
			stack2 = []
			sTmp = ''
			for index in range(len(s)):
				if s[index] == ','  and stack == [] and stack2 == []:
					mysplit.append(sTmp.strip())
					sTmp = ''
					continue
				if s[index] == '\'' or s[index] == '\"':
					if stack2 == []:
						stack2.append(s[index])
					else:
						if s[index] == stack2[0]:
							stack2 = []
						else:
							stack2.append(s[index])
				if s[index] == '{' and stack2 == []:
					stack.append('{')
				if s[index] == '}' and stack2 == []:
					del stack[-1]
				sTmp += s[index]
				if index == len(s) - 1:
					mysplit.append(sTmp.strip())
			list2tuple = []
			for k in mysplit:
				list2tuple.append(self.string2other(k))
			return tuple(list2tuple)

		elif s[0] == '-' or s[0] == '+':
			sign_flag = 1
			for i in range(len(s)):
				if s[i] == '-' or s[i] == '+' or s[i] == ' ':
					if s[i] == '-':
						sign_flag *= -1
				else:
					s = s[i:]
					break
			float_flag = 0
			for t in s:
				if t == '.' or t == 'e' or t == 'E' or s[-1] == 'f' or s[-1] == 'F':
					float_flag = 1
				if s[:2] == '0x' or s[:2] == '0X':
					float_flag = 0
			if float_flag == 0:
				if len(s) > 4 and (s[-3:] == 'i64' or s[-3:] == 'I64'):
					s = s[:-3]
				elif len(s) > 3 and (s[-2:] == 'll' or s[-2:] == 'LL'):
					s = s[:-2]
				elif len(s) > 2 and (s[-1] == 'l' or s[-1] == 'L'):
					s = s[:-1]
				if s[0] == '0':
					if s == '0':
						return int(s)*sign_flag
					elif s[1] == 'x' or s[1] == 'X':
						return int(s, 16)*sign_flag
					elif s[1] == 'b' or s[1] == 'B':
						return int(s, 2)*sign_flag
					else:
						return int(s, 8)*sign_flag
				else:
					return int(s)*sign_flag
			else:
				if (s[-1] == 'f' or s[-1] == 'l' or s[-1] == 'F' or s[-1] == 'L'):
					s = s[:-1]
				return float(s)*sign_flag
		else:
			float_flag = 0
			for t in s:
				if t == '.' or t == 'e' or t == 'E' or s[-1] == 'f' or s[-1] == 'F':
					float_flag = 1
				if s[:2] == '0x' or s[:2] == '0X':
					float_flag = 0
			if float_flag == 0:
				if len(s) > 4 and (s[-4:] == 'ui64' or s[-4:] == 'uI64' or s[-4:] == 'Ui64' or s[-4] == 'UI64'):
					s = s[:-4]
				elif len(s) > 3 and (s[-3:] == 'i64' or s[-3:] == 'I64' or s[-3:] == 'ull' or s[-3:] == 'uLL' or s[-3:] == 'Ull' or s[-3:] == 'ULL'):
					s = s[:-3]
				elif len(s) > 2 and (s[-2:] == 'll' or s[-2:] == 'LL' or s[-2:] == 'ul' or s[-2:] == 'Ul' or s[-2:] == 'uL' or s[-2:] == 'UL'):
					s = s[:-2]
				elif (s[-1] == 'u' or s[-1] == 'U' or s[-1] == 'l' or s[-1] == 'L'):
					s = s[:-1]
				if s[0] == '0':
					if s == '0':
						return int(s)
					elif s[1] == 'x' or s[1] == 'X':
						return int(s, 16)
					elif s[1] == 'b' or s[1] == 'B':
						return int(s, 2)
					else:
						return int(s, 8)
				else:
					return int(s)
			else:
				if (s[-2:] == 'lf' or s[-2:] == 'LF' or s[-2:] == 'lF' or s[-2:] == 'Lf'):
					s = s[:-2]
				if (s[-1] == 'f' or s[-1] == 'l'
						or s[-1] == 'F' or s[-1] == 'L'):
					s = s[:-1]
				tt = s.split(' ')
				s = ''
				for k in tt:
					s += k
				return float(s)

	def DictDataConversion(self, d):
		for key in d:
			d[key] = self.string2other(d[key])
		return d

	def DumpDict(self):
		dictconv = {}
		for key in self.dict:
			dictconv[key] = self.dict[key]
		dictconv = self.DictDataConversion(dictconv)
		return dictconv

#global varable
BackupWorkPath = ''
TextSrcFd = 0
BinaryFd = 0
BinOutFileStr = ''
TextDeviceFd = 0
TextPropertyFd = 0
MajorStr = 0
MinorStr = 0
PropertyStartAddrNum = 0
TextSrcSplitList = []
DeviceNameList = []
VersionPos = 0
PropertyNameList = []

StateStr = 'PropertyName'  # state mechine : PropertyName -> PropertyContent -> PropertyName
PropertyTotalLength = 0  # property ID,element size, content length,property content -> total size
ElementSize = 0 		 # property element size (data type size) 
PropertyConentLength = 0  # property conent length
DevicePropertyLengthList = [] # all of device's property total length 
PropertyBinList = []		# single property binary collection
ElementSizeList = []        # single property element size collection 
PropertyConentECList = []   # single element count property content collection  
PropertyContentList = []    # multi element count property content collection  

AccPropertyLengthList = []	# every device use how many property block size 
DevicePropertyCountList = [] # How many property names the device has
DevicePropertyBinList = [] # single device(multi property name)'s property binary collection

TotalDefinelist = []  # all of header file content after HeaderParser conversion 
TotalSizePos = 0  # DCM header : total binary size
LastFilePos = 0 # total binray file size

# open file (python source file, text source file , binary file)
def OpenFile():
	global TextSrcFd,BinaryFd,TextDeviceFd,TextPropertyFd,BinOutFileStr,BackupWorkPath

	ArgvLen = 2
	InputCmdList = sys.argv
	if len(InputCmdList) != ArgvLen:
		print "!! error input command !!"
		print "Example : python PySysDesc.py infinity6-ssc009b-s01a.sys"
		print "PySysDesc.py - python source file"
		print "infinity6-ssc009b-s01a.sys - system descripition source file"
		return False

	# change python work folder
	BackupWorkPath = os.getcwd()
	SysPath = InputCmdList[1]
	SysPath = SysPath[1:]  # cancel .
	TempList = SysPath.split('/')
	SysFileName = TempList[len(TempList)-1]
	SysFileNameLen = len(SysFileName)
	NewWorkPath = SysPath[:0-SysFileNameLen-1]
	NewWorkPath = BackupWorkPath + NewWorkPath
	os.chdir(NewWorkPath)
	TextSrcFileStr = SysFileName
	TempStr = TextSrcFileStr.split('.')
	BinOutFileStr = TempStr[0] + '.bin'
	TextSrcFd = open(TextSrcFileStr,'r')
	BinaryFd = open(BinOutFileStr,'wb')
	TextDeviceFd = open('device_id.h','w')
	TextPropertyFd = open('property_id.h','w')
	return True

def RemoveComment(String):
	TempStr = String
	while True:
		isComment = TempStr.find('//')
		if isComment >= 0:
			HeadStr = TempStr[:isComment]
			TempStr = TempStr[isComment:]
			isCarriageReturn = TempStr.find('\r\n')
			if isCarriageReturn >= 0:
				TailStr = TempStr[isCarriageReturn:]
				TempStr = HeadStr + TailStr
		else:
			break
	while True:
		isComment = TempStr.find('/*')
		if isComment >= 0:
			HeadStr = TempStr[:isComment]
			TempStr = TempStr[isComment:]
			isCarriageReturn = TempStr.find('*/')
			if isCarriageReturn >= 0:
				TailStr = TempStr[isCarriageReturn+2:]
				TempStr = HeadStr + TailStr
		else:
			break
	#print TempStr
	return TempStr

def GetVersionInfo():
	global TextSrcFd,TextSrcSplitList,MajorStr,MinorStr

	TextSrcFd.seek(0) # move file pointer in start position
	# check version info 
	RawStr = TextSrcFd.read()
	TextStr = RemoveComment(RawStr)
	#print TextStr
	TextSrcSplitList = TextStr.split()
	#print TextSrcSplitList
	VersionIndex = TextStr.find('version :',0,len(TextStr))
	if VersionIndex == -1:
		print "!! No any version info or syntex error  !!"
		print "!! Standard pattern is version : 01.01  !!"
		return False
	VersionEndIndex = TextStr.find('\n',VersionIndex,len(TextStr))
	VersionStr = TextStr[VersionEndIndex-6:VersionEndIndex]
	MajorStr = VersionStr[0:2]
	MinorStr = VersionStr[3:5]
	#print ('Major=%s Minor=%s' % (MajorStr,MinorStr))
	return True

def PickupDeviceName():
	global DeviceNameList,TextSrcSplitList,VersionPos
	
	DeviceNameVerifyList = []
	for i in range(0,len(TextSrcSplitList),1):
		if TextSrcSplitList[i] == 'version':
			break
	VersionPos = i
	for i in range(0,len(TextSrcSplitList),1):
		TempStr = TextSrcSplitList[i]
		if (TempStr[0] == '<') and (TempStr[-1] == '>') and (i > VersionPos):
			DeviceNameList.append(TempStr)
	# whether the device name is duplicate
	DeviceNameVerifyList =  copy.deepcopy(DeviceNameList)
	DeviceNameVerifyList.sort()
	for i in range(0,len(DeviceNameVerifyList)-1,1):
		if DeviceNameVerifyList[i] == DeviceNameVerifyList[i+1]:
			print "!! device name:" + DeviceNameVerifyList[i] + " is duplicate , isn't single !!"
			return False
	#check device naming rule follow C language <--- TODO item
	#print DeviceNameList
	return True

def SourceCodeLicense(fd):
	fd.write("/* SigmaStar trade secret */\n")
	fd.write("/* Copyright (c) [2019~2020] SigmaStar Technology.\n")
	fd.write("All rights reserved.\n")
	fd.write("\n")
	fd.write("Unless otherwise stipulated in writing, any and all information contained\n")
	fd.write("herein regardless in any format shall remain the sole proprietary of\n")
	fd.write("SigmaStar and be kept in strict confidence\n")
	fd.write("(SigmaStar Confidential Information) by the recipient.\n")
	fd.write("Any unauthorized act including without limitation unauthorized disclosure,\n")
	fd.write("copying, use, reproduction, sale, distribution, modification, disassembling,\n")
	fd.write("reverse engineering and compiling of the contents of SigmaStar Confidential\n")
	fd.write("Information is unlawful and strictly prohibited. SigmaStar hereby reserves the\n")
	fd.write("rights to any and all damages, losses, costs and expenses resulting therefrom.\n")
	fd.write("*/\n")
	fd.write("\n")

def CreateDeviceID():
	global DeviceNameList,TextDeviceFd
	
	DeviceNameSeqList = []
	for i in range(0,len(DeviceNameList),1):
		TempStr = DeviceNameList[i]
		TempStr = TempStr[1:]   # cancel '<'
		TempStr = TempStr[:-1]  # cancel '>'
		TempStr = '#define SYSDESC_DEV_' + TempStr + ' ' + str(hex(i*2)) + '\n'
		DeviceNameSeqList.append(TempStr)
	SourceCodeLicense(TextDeviceFd)
	DeviceNameSeqList.append('\n')
	DeviceNameSeqList.append('#define SYSDESC_DEV_MIN 0x0\n')
	TempStr = '#define SYSDESC_DEV_MAX ' + str(hex(i*2)) + '\n'
	DeviceNameSeqList.append(TempStr)
	TextDeviceFd.writelines(DeviceNameSeqList)
	TextDeviceFd.close()

def CreatePropertyID():
	global TextSrcSplitList,VersionPos,PropertyNameList,TextPropertyFd
	
	OriginalPropertyNameList = []
	PropertyNameSeqList = []
	for i in range(0,len(TextSrcSplitList),1):
		TempStr = TextSrcSplitList[i]
		if (TempStr[0] == '[') and (TempStr[-1] == ']') and (i > VersionPos):
			OriginalPropertyNameList.append(TempStr)
	OriginalPropertyNameList.sort()
	#check property naming rule follow C language <--- TODO item 
	#delete duplicate property naming
	for i in range(0,len(OriginalPropertyNameList)-1,1):
		if OriginalPropertyNameList[i] != OriginalPropertyNameList[i+1]:
			PropertyNameList.append(OriginalPropertyNameList[i])
	PropertyNameList.append(OriginalPropertyNameList[-1])	
	for i in range(0,len(PropertyNameList),1):
		TempStr = PropertyNameList[i]
		TempStr = TempStr[1:]   # cancel '['
		TempStr = TempStr[:-1]  # cancel ']'
		TempStr = '#define SYSDESC_PRO_' + TempStr + ' ' + str(hex(i)) + '\n'
		PropertyNameSeqList.append(TempStr)
	SourceCodeLicense(TextPropertyFd)
	PropertyNameSeqList.append('\n')
	PropertyNameSeqList.append('#define PROPERTY_MIN 0x0\n')
	TempStr = '#define PROPERTY_MAX ' + str(hex(i)) + '\n'
	PropertyNameSeqList.append(TempStr)
	TextPropertyFd.writelines(PropertyNameSeqList)
	TextPropertyFd.close()
	#print PropertyNameList

def CreateBinaryHeader():
	global BinaryFd,MajorStr,MinorStr,DeviceNameList,PropertyStartAddrNum,TotalSizePos
	
	MagicNum = 0xbabe5aa5
	MagicBin = struct.pack("I",MagicNum)
	BinaryFd.write(MagicBin)
	MajorNum = int(MajorStr,10)
	MinorNum = int(MinorStr,10)
	MajorBin = struct.pack("B",MajorNum)
	MinorBin = struct.pack("B",MinorNum)
	BinaryFd.write(MinorBin)
	BinaryFd.write(MajorBin)
	DeviceStartAddrNum = 0x000c
	DeviceStartAddrBin = struct.pack("H",DeviceStartAddrNum)
	BinaryFd.write(DeviceStartAddrBin)
	PropertyStartAddrNum = DeviceStartAddrNum + (len(DeviceNameList) *2)
	PropertyStartAddrBin = struct.pack("H",PropertyStartAddrNum)
	BinaryFd.write(PropertyStartAddrBin)
	TotalSizeNum = 0x0000  # Wait for the confirmation of the total number of binary count <-- TODO item 
	TotalSizeBin = struct.pack("H",TotalSizeNum)
	TotalSizePos = BinaryFd.tell()
	BinaryFd.write(TotalSizeBin)
	if DeviceStartAddrNum != BinaryFd.tell():
		print "!! The binary file is written to the device index block in the wrong position !!"
		return False
	else:
		return True
	
def PropertyNameParsing(InputStr,PropertyEncodeBinList):
	global PropertyNameList,PropertyTotalLength,ElementSize,PropertyBinList,StateStr,ElementSizeList

	if StateStr != 'PropertyName':
		print "!! state mechine error in PropertyNameParsing() !!"
		return False

	for i in range (0,len(PropertyNameList),1):
		if InputStr == PropertyNameList[i]:   # find out the property name match with PropertyNameList
			break
	PropertyBinList = []	# clear PropertyBinList, PropertyBinList holds the contents of a property name
	PropertyBinList.append(PropertyEncodeBinList[i])  # insert property ID binary
	PropertyTotalLength = PropertyTotalLength + 2 + 2 + 2 	# property ID = 2 bytes , element size = 2 bytes , content length = 2 bytes + property content = ?
	TempStr = InputStr
	TempStr = TempStr[:-1]
	PropertyNamingSplitList = TempStr.split('_')
	IsString = 0
	for i in range(0,len(PropertyNamingSplitList),1):   # calculate the element size 
		if PropertyNamingSplitList[i] == 'u32':
			if IsString == 1:
				ElementSize = 0
				break
			ElementSize = ElementSize + 4
			ElementSizeList.append(4)
		elif PropertyNamingSplitList[i] == 'u16':
			if IsString == 1:
				ElementSize = 0
				break
			ElementSize = ElementSize + 2
			ElementSizeList.append(2)
		elif PropertyNamingSplitList[i] == 'u8':
			if IsString == 1:
				ElementSize = 0
				break
			ElementSize = ElementSize + 1
			ElementSizeList.append(1)
		elif PropertyNamingSplitList[i] == 'str':
			if IsString == 1:
				ElementSize = 0
				break
			ElementSize = ElementSize + 1
			ElementSizeList.append(1)
			IsString = 1
		else:
			if ElementSize > 0:
				print "!! property naming rule is invalid in " + TempStr + " !!"
				return False
	if ElementSize == 0:
		print "!! property naming rule is invalid in " + TempStr + " !!"
		print "!! 1.No any data type !!"
		print "!! 2.Data type mixed u8/u16/u32 and strings !!"
		return False
	StateStr = 'PropertyContent'
	PropertyBinList.append(struct.pack("H",ElementSize))   # insert element size binary
	return True

def FindDefineValue(string):
	global TotalDefinelist
	
	TmpDict = {}
	Result = -1 
	for i in range(0,len(TotalDefinelist),1):
		TmpDict = TotalDefinelist[i]
		TmpKey = TmpDict.get(string)
		if TmpKey == None:
			continue
		else:
			Result = TmpKey
			break
	return Result 

def GroupIdentifierParsing(InputStr):
	global ElementSize,PropertyConentLength,StateStr,ElementSizeList,PropertyConentECList,PropertyContentList
		
	if StateStr != 'PropertyContent':
		print "!! state mechine error in GroupIdentifierParsing() !!"
		return False

	TempStr = InputStr
	PropertyConentECList.append(TempStr[0:-1])
	if len(PropertyConentECList) != len(ElementSizeList):
		print "!! element count isn't match !!"
		return False

	for i in range(0,len(PropertyConentECList),1):
		TempStr = PropertyConentECList[i]
		if TempStr.isdigit() == True:
			num = int(TempStr,10)
			if num < math.pow(2,ElementSizeList[i]*8):
				PropertyContentList.append(num)
			else:
				print "!! out of data type range" + TempStr + "!!"
				return False
		elif (TempStr[0:2] == '0x') or (TempStr[0:2] == '0X'):
			num = int(TempStr,16)
			if num < math.pow(2,ElementSizeList[i]*8):
				PropertyContentList.append(num)
			else:
				print "!! out of data type range" + TempStr + "!!"
				return False
		else:
			num = FindDefineValue(TempStr)
			if num == -1:
				print "!! (" + TempStr + ") , Can't find out the property content from header file !!"
				return False
			if num < math.pow(2,ElementSizeList[i]*8):
				PropertyContentList.append(num)
			else:
				print "!! out of data type range" + TempStr + "!!"
				return False
	StateStr = 'PropertyContent'
	PropertyConentECList = []
	PropertyConentLength = PropertyConentLength + ElementSize
	return True

def EndIdentifierParsing(InputStr):
	global ElementSize,PropertyConentLength,PropertyTotalLength,PropertyBinList,StateStr,ElementSizeList,PropertyConentECList,PropertyContentList

	if StateStr != 'PropertyContent':
		print "!! state mechine error in EndIdentifierParsing() !!"
		return False

	DoubleQuotesCount = 0
	String=''
	TempStr = InputStr
	PropertyConentECList.append(TempStr[0:-1])
	for i in range(0,len(PropertyConentECList),1):    # check String property content
		TempStr = PropertyConentECList[i]
		for j in range(0,len(TempStr),1):
			if TempStr[j] == '"':
				DoubleQuotesCount = DoubleQuotesCount + 1
			if DoubleQuotesCount > 0:
				String = String + TempStr[j]
		String = String + ' '    # insert space key
	if not(DoubleQuotesCount == 0 or DoubleQuotesCount == 2):
		print "!! Incorrect number of double quotes !!"
		return False
	if (len(PropertyConentECList) != len(ElementSizeList)) and (DoubleQuotesCount == 0):
		print "!! element count isn't match in EndIdentifierParsing() !!"
		return False

	if DoubleQuotesCount == 2:
		AsciiStr = String[1:-2]   # cancel " "
		for i in range(0,len(AsciiStr),1):
			num = ord(AsciiStr[i])  # 0~127
			PropertyContentList.append(num)
			PropertyConentLength = PropertyConentLength + 1
		num = 0		# insert \0 = null
		PropertyContentList.append(num)
		PropertyConentLength = PropertyConentLength + 1
	else:
		for i in range (0,len(PropertyConentECList),1):
			TempStr = PropertyConentECList[i]
			if TempStr.isdigit() == True:
				num = int(TempStr,10)
				if num < math.pow(2,ElementSizeList[i]*8):
					PropertyContentList.append(num)
				else:
					print "!! out of data type range" + TempStr + "!!"
					return False
			elif (TempStr[0:2] == '0x') or (TempStr[0:2] == '0X'):
				num = int(TempStr,16)
				if num < math.pow(2,ElementSizeList[i]*8):
					PropertyContentList.append(num)
				else:
					print "!! out of data type range" + TempStr + "!!"
					return False
			else:
				num = FindDefineValue(TempStr)
				if num == -1:
					print "!! (" + TempStr + ") , Can't find out the property content from header file !!"
					return False
				if num < math.pow(2,ElementSizeList[i]*8):
					PropertyContentList.append(num)
				else:
					print "!! out of data type range" + TempStr + "!!"
					return False
		PropertyConentLength = PropertyConentLength + ElementSize
	PropertyConentECList = []
	PropertyTotalLength = PropertyTotalLength + PropertyConentLength
	PropertyBinList.append(struct.pack("H",PropertyConentLength))   # insert content length binary
	PropertyConentLength = 0
	ElementSize = 0

	for i in range (0,len(PropertyContentList),1):
		num = i % len(ElementSizeList)
		if ElementSizeList[num] == 4:
			PackFormat = "I"
		elif ElementSizeList[num] == 2:
			PackFormat = "H"
		else:	
			PackFormat = "B"
		PropertyBinList.append(struct.pack(PackFormat,PropertyContentList[i]))  # insert property content binary

	#print PropertyBinList
	DevicePropertyBinList.append(PropertyBinList)
	ElementSizeList = []
	PropertyContentList = []
	StateStr = 'PropertyName'
	return True

def KernelProcess():
	global PropertyNameList,TextSrcSplitList,PropertyTotalLength,ElementSize,DevicePropertyLengthList,DeviceNameList,ElementSizeList,PropertyConentECList,DevicePropertyCountList

	PropertyEncodeBinList = []
	DeviceIndexList = []
	DevicePropertyCount = 0

	# encode property name 
	for i in range(0,len(PropertyNameList),1):
		PropertyEncodeBinList.append(struct.pack("H",i))
	#print PropertyEncodeBinList

	# get text string index 
	for i in range(0,len(DeviceNameList),1):
		for j in range(0,len(TextSrcSplitList),1):
			if DeviceNameList[i] == TextSrcSplitList[j]:
				DeviceIndexList.append(j)
	#print DeviceIndexList

	for i in range(0,len(DeviceIndexList),1):
		if i < len(DeviceIndexList)-1:    # -1 : avoid out of range in DeviceIndexList[i+1]
			for j in range(DeviceIndexList[i]+1,DeviceIndexList[i+1],1): # DeviceIndexList represent the index between device name , DeviceIndexList[i]+1 : after <XX> , DeviceIndexList[i+1] : before <XX>
				TempStr = TextSrcSplitList[j]
				if (TempStr[0] == '[') and (TempStr[-1] == ']'):  # is property name
					if False == PropertyNameParsing(TempStr,PropertyEncodeBinList):
						return False
					else:
						DevicePropertyCount = DevicePropertyCount + 1
				elif TempStr[-1] == ',':   # is group identifier  if 0x23ZZ or 0x1234 or ',' we can't watch out this case   <-- TODO item
					if TempStr.find('"') >= 0:
						print "!! The property content don't support the multi-element in string data type. !!"
						return False
					if False == GroupIdentifierParsing(TempStr):
						return False
				elif TempStr[-1] == ';':   # is end identifier    if 0x1234 of 0x23ZZ of ';' we can't watch out this case  <-- TODO item
					if False == EndIdentifierParsing(TempStr):
						return False
				else:
					PropertyConentECList.append(TempStr)
			DevicePropertyLengthList.append(PropertyTotalLength)
			PropertyTotalLength = 0
			DevicePropertyCountList.append(DevicePropertyCount)
			DevicePropertyCount = 0
		else : # last device ID
			PropertyTotalLength = 0
			ElementSize = 0
			PropertyConentLength = 0
			for j in range(DeviceIndexList[i]+1,len(TextSrcSplitList),1):
				TempStr = TextSrcSplitList[j]
				if (TempStr[0] == '[') and (TempStr[-1] == ']'):  # is property name
					if False == PropertyNameParsing(TempStr,PropertyEncodeBinList):
						return False
					else:
						DevicePropertyCount = DevicePropertyCount + 1
				elif TempStr[-1] == ',':   # is group identifier  if 0x23ZZ or 0x1234 or ',' we can't watch out this case   <-- TODO item
					if TempStr.find('"') >= 0:
						print "!! The property content don't support the multi-element in string data type. !!"
						return False
					if False == GroupIdentifierParsing(TempStr):
						return False
				elif TempStr[-1] == ';':   # is end identifier    if 0x1234 of 0x23ZZ of ';' we can't watch out this case  <-- TODO item
					if False == EndIdentifierParsing(TempStr):
						return False
				else:
					PropertyConentECList.append(TempStr)
			DevicePropertyLengthList.append(PropertyTotalLength)
			PropertyTotalLength = 0
			DevicePropertyCountList.append(DevicePropertyCount)
			DevicePropertyCount = 0
	return True

def CreateDeviceIndexBlock():
	global DevicePropertyLengthList,DeviceNameList,BinaryFd,PropertyStartAddrNum,AccPropertyLengthList

	#print DevicePropertyLengthList
	FirstPropertyFlag = 0
	LastValidIndex = 0
	for i in range(0,len(DeviceNameList),1):
		if DevicePropertyLengthList[i] == 0:
			AccPropertyLengthList.append(0)
		else:
			if FirstPropertyFlag == 0:
				FirstPropertyFlag = 1
				AccPropertyLengthList.append(PropertyStartAddrNum)
			else:
				AccPropertyLengthList.append(AccPropertyLengthList[LastValidIndex]+DevicePropertyLengthList[LastValidIndex])
			LastValidIndex = i

	#print AccPropertyLengthList

	for i in range(0,len(AccPropertyLengthList),1):
		DeviceIndexBlockBin = struct.pack("H",AccPropertyLengthList[i])
		BinaryFd.write(DeviceIndexBlockBin)

def CreatePropertyBlock():
	global BinaryFd,DevicePropertyCountList,DevicePropertyBinList,AccPropertyLengthList,PropertyStartAddrNum

	Base = 0
	TempBinList = []
	if PropertyStartAddrNum != BinaryFd.tell():
		print "!! The binary file is written to the property block in the wrong position !!"
		print "!! Please check property start address !!"
		return False

	#print 'DevicePropertyCountList'
	#print 'DevicePropertyBinList'
	for i in range(0,len(AccPropertyLengthList),1):
		if BinaryFd.tell() == AccPropertyLengthList[i]:   # check binary file fp = AccPropertyLengthList[i]
			for j in range(Base,DevicePropertyCountList[i]+Base,1):
				TempBinList.append(DevicePropertyBinList[j])
			Base = Base + DevicePropertyCountList[i]
			for j in range(0,len(TempBinList),1):
				for k in range(0,len(TempBinList[j]),1):
					BinaryFd.write(TempBinList[j][k])
			TempBinList = []
		else:
			print "!! The binary file is written to the property block in the wrong position !!"
			return False
	return True

def SupportHeaderFile():
	global TextSrcFd,TotalDefinelist

	C_Preprocessors = HeaderParser()
	PathList = []
	ConversionDict = {}
	SrcList = TextSrcFd.readlines()
	for i in range(0,len(SrcList),1):
		TmpStr = SrcList[i]
		TmpStr = TmpStr.strip()
		if TmpStr.find('version :') >= 0:
			break
		if TmpStr.find('#include') >= 0:
			LeftPos = TmpStr.find('<',0,len(TmpStr))
			RightPos = TmpStr.find('>',0,len(TmpStr))
			if LeftPos == -1 or RightPos == -1:
				print "!!! syntex error : #include format in " + TmpStr + "!!!"
				return False
			else:
				PathStr = TmpStr[LeftPos+1:RightPos]
				PathList.append(PathStr)

	for i in range(0,len(PathList),1):
		Result = C_Preprocessors.LoadFile(PathList[i])
		if Result == True:
			ConversionDict = C_Preprocessors.DumpDict()
			#print ConversionDict
			TotalDefinelist.append(ConversionDict)
		else:
			print "!!! Parser the header file : "+ PathList[i] + " happend the error condition !!!"
			return False
	return True

def BackFillTotalSize():
	global BinaryFd,TotalSizePos,LastFilePos

	LastFilePos = BinaryFd.tell()
	#print ('LastFilePos=%d' % (LastFilePos))
	TotalSizeBin = struct.pack("H",LastFilePos)
	BinaryFd.seek(TotalSizePos)
	BinaryFd.write(TotalSizeBin)
	BinaryFd.close()

def ConversionConstArray():
	global LastFilePos,BinOutFileStr

	BinConstArrayFd = open('hal_sysdesc.h','w')
	TempBinaryFd = open(BinOutFileStr,'rb')
	ConstArrayStr = ''
	TempStr = ''
	SourceCodeLicense(BinConstArrayFd)
	BinConstArrayFd.write("\n")
	BinConstArrayFd.write("const unsigned char u8SysDesTable[] =")
	BinConstArrayFd.write("{\n")
	j = 0
	for i in range(0,LastFilePos,1):
		OneByte = TempBinaryFd.read(1)
		TempNum = struct.unpack("B",OneByte)
		TempStr = hex(TempNum[0])
		if i == (LastFilePos - 1):
			ConstArrayStr = ConstArrayStr + TempStr
		else:
			ConstArrayStr = ConstArrayStr + TempStr +','
		j = j + 1
		if j >= 16:
			ConstArrayStr = ConstArrayStr + '\r\n'
			j = 0
	#print ConstArrayStr
	BinConstArrayFd.write(ConstArrayStr)
	BinConstArrayFd.write("};")
	BinConstArrayFd.close()
	TempBinaryFd.close()

# start entry , don't change the call flow 
print "Build system descriptor file"
if OpenFile() == False:
	print "Failed in call OpenFile()"
	os._exit(1)
if SupportHeaderFile() == False:
	print "Failed in call SupportHeaderFile()"
	os._exit(1)
if GetVersionInfo() == False:
	print "Failed in call GetVersionInfo()"
	os._exit(1)
if PickupDeviceName() == False:
	print "Failed in call PickupDeviceName()"
	os._exit(1)
CreateDeviceID()
CreatePropertyID()
if CreateBinaryHeader() == False:
	print "Failed in call CreateBinaryHeader()"
	os._exit(1)
if KernelProcess() == False:
	print "Failed in call KernelProcess()"
	os._exit(1)
CreateDeviceIndexBlock()
if CreatePropertyBlock() == False:
	print "Failed in call CreatePropertyBlock()"
	os._exit(1)
BackFillTotalSize()
ConversionConstArray()
TextSrcFd.close()
os.remove(BinOutFileStr)
os.chdir(BackupWorkPath)
print "Build system descriptor file : Pass"
os._exit(0)
