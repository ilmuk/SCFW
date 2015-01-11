#pragma once
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include "structs.cpp"


using namespace std; 


class VHDLParser
{
   public:
	  vector<module> mods;
	  vector<strct> structs;
	  vector<string> error_list;

	  /*
	  *	  Конструктор класса VHDL-парсера. Работает в двух режимах:
	  *
	  *	  + mode = 0 - параметр str воспринимается как название файла с VHDL кодом, далее входной поток берется оттуда
	  *	  + mode != 0 - параметр str воспринимается как VHDL код
	  */
	  VHDLParser(string str,int mode)
	  {
		  string_readed = false;

		  switch( mode )
		  {
			  case 0:
				  this->file = ifstream(str.c_str());
				  break;

			  default:
				  this->str = str;
		  }

		  this->mode = mode;
	  }

	  /*
	  *		Создание структурной модели VHDL кода. Как результат работы функции формируется:
	  *	  
	  *		+ self::mods - набор модулей(компонентов) найденных в коде
	  *		+ self::structs - набор заданных пользовательских типов данных
	  */
	  bool MakeModel()
	  {
		  string cur_token;

		  while(getToken(cur_token))
		  {
			  switch(detectConstruct(trim(cur_token)))
			  {
				  case UNDEF_CONSTR:
					  error_list.push_back("Warning: undefine token '"+cur_token+"'");
					  break;
				  case MODEL_ARCH_CONSTR:
					  makeArchModel(cur_token);
					  break;
				  case MODEL_IMPL_CONSTR:
					  makeImplModel(cur_token);
					  break;
				  case TYPE_CONSTR:
					  makeType(cur_token);
					  break;
				  default:
					  error_list.push_back("Warning: undefine construct key for token '"+cur_token+"'");

			  }
		  }

		  return true;
	  }
 
   private:
      ifstream file;				// Файловый дескриптор для работы парсера в режиме 0
	  int mode;						// Номер режима работы парсера
	  string str;					// Входная переменная str конструктора парсера
	  bool string_readed;			// Вспомогательная переменная для облегчения совмещения режимов работы
	  vector<string> tokenlist;		// Список подгруженных токенов из входного потока
	  vector<string> stringlist;	// Список подгруженных строк из входного потока

	  /*
	  *		Константы
	  */

	  static const int UNDEF_CONSTR = 0;		// Неизвестное значение токена	
	  static const int MODEL_ARCH_CONSTR = 1;	// Определено начало описание интерфейса модуля
	  static const int MODEL_IMPL_CONSTR = 2;	// Определено начало описания реализации модуля
	  static const int TYPE_CONSTR = 3;			// Определено начало задания пользовательского типа данных

	  static const int ARCH_PORT_CONSTR = 2;	// Определено начало описания порта интерфейса модуля
	  static const int ARCH_END_CONSTR = 3;		// Определено окончание описания интерфейса модуля

	  /*
	  *		Парсер интерфейса модуля. Как результат работы, в случае корректного построения VHDL кода, добавляет в self::mods модуль с заданным интерфейсом
	  */
	  void makeArchModel(const string cur_token)
	  {
		  module new_module;

		  regex rx("^entity[ \r\n\t\\(]*([^ \r\n\t\\)]*)",regex::icase);
		  smatch m;

		  regex_search(cur_token.begin(),cur_token.end(),m,rx);

		  new_module.name="";

		  if(m.size() >= 2)
			new_module.name = m[1];

		  if(new_module.name == "")
		  {
			  error_list.push_back("Error: Module name is not found in token '"+cur_token+"'");
			  return;
		  }


		  string token;
		  bool exit_entity = false;
		  vector<port> p;

		  while(!exit_entity && getToken(token))
		  {
			  switch(detectArchConstr(token))
			  {
				  case UNDEF_CONSTR:
					  error_list.push_back("Warning: undefine token '"+token+"' for entity '"+new_module.name+"'");
					  break;
				  case ARCH_PORT_CONSTR:
					  p = MakePortDefine(token);
					  new_module.ports.insert(new_module.ports.end(),p.begin(),p.end());
					  break;
				  case ARCH_END_CONSTR:
					  if(CheckCorrectEnd(token,new_module.name))
					  {
						  exit_entity = true;
					  }
					  else
					  {
						  error_list.push_back("Warning: undefine END token in line'"+token+"' for entity '"+new_module.name+"'");
					  }
					  break;
				  default:
					  error_list.push_back("Warning: undefine construct key for token '"+token+"' for entity '"+new_module.name+"'");

			  }
		  }
		  mods.push_back(new_module);
	  }

	  /*
	  *		Парсер реализации модуля. Как результат работы, в случае корректного построения VHDL кода, добавляет в уже существующий в self::mods модуль реализацию.
	  */
	  void makeImplModel(string cur_token)
	  {

	  }

	  /*
	  *		Парсер пользовательского типа. Как результат работы, в случае корректного построения VHDL кода, добавляет в self::stucts пользовательский тип данных
	  */
	  void makeType(const string cur_token)
	  {
		  strct new_type;

		  regex rx("^type[ \r\n\t\\(]*([^ \r\n\t\\)]*)",regex::icase);
		  smatch m;

		  regex_search(cur_token.begin(),cur_token.end(),m,rx);

		  new_type.name="";

		  if(m.size() >= 2)
			new_type.name = m[1];

		  if(new_type.name == "")
		  {
			  error_list.push_back("Error: Type name is not found in token '"+cur_token+"'");
			  return;
		  }

		  string token;
		  if(!getToken(token))
		  {
			  error_list.push_back("Error: Type definition is not complite when getting EOF");
			  return;
		  }

		  const string s_token = token;
		  regex rx2("^[ \r\n\t]*record[ \r\n\t]*",regex::icase);

		  if(regex_search(s_token.begin(),s_token.end(),m,rx2)) // record
		  {
			  new_type.components = MakeRecordDefine(s_token);
		  }
		  else
		  { // single type define
			  token = new_type.name + " : " + token;
			  vector<port> tmp;
			  ParseSignals(token,tmp,true);
			  new_type.dtp=tmp[0].dtp;
		  }

		  structs.push_back(new_type);

	  }

	  /*
	  *		Парсер структуры пользовательского типа данных
	  */
	  vector<port> MakeRecordDefine(string cur_token)
	  {
		  vector<port> out;
		  string signalstring="";

		  if(cur_token.find(":") != string::npos)
		  {
			  string empty_str="";
			  regex rx("^[\n\t\r ]*record[\n\t\r ]*[\\(\n\t\r ]*",regex::icase);

		      signalstring = regex_replace(cur_token,rx,empty_str);

			  ParseSignals(signalstring,out,true);
		  }

		  bool ended = false;

		  while(getToken(signalstring))
		  {
			  regex rx("^[\n\t\r ]*end[\n\t\r ]*record*",regex::icase);
			  const string c_sig=signalstring;
			  smatch tmp;
			  if(regex_search(c_sig.begin(),c_sig.end(),tmp,rx))
			  {
				  ended = true;
				  break;
			  }

			  ParseSignals(signalstring,out,true);
		  }

		  if(!ended)
			error_list.push_back("Error: record is not complite when getting EOF");

		  return out;
	  }

	  /*
	  *		Парсер порта
	  */
	  vector<port> MakePortDefine(string cur_token)
	  {

		  vector<port> out;
		  string signalstring="";

		  if(cur_token.find(":") != string::npos)
		  {
			  string empty_str="";
			  regex rx("^[\n\t\r ]*port[\n\t\r ]*\\([\n\t\r ]*",regex::icase);

		      signalstring = regex_replace(cur_token,rx,empty_str);

			  ParseSignals(signalstring,out,false);
		  }
		  
		  while(!portExit(signalstring) && getToken(signalstring))
			  ParseSignals(signalstring,out,false);

		  if(!portExit(signalstring))
			error_list.push_back("Error: port is not complite when getting EOF");

		  return out;
	  }

	  /*
	  *		Парсер сигнала
	  */
	  void ParseSignals(string signalstring,vector<port>& crport,bool without_type)
	  {

		  vector<string> def_parts = explode(":",signalstring);
		  if(def_parts.size() != 2)
		  {
			  error_list.push_back("Warning: invalid signal definition in line '"+signalstring+"'");

			  return;
		  }

		  string firstw = getFirstWord(signalstring);
		  transform(firstw.begin(),firstw.end(), firstw.begin(), ::tolower);
		  
		  unsigned char ptype=255;
		  if(!without_type)
		  {
			  if(firstw == "signal")
			  {
				  ptype = P_SIGNAL;

				  string empty_str="";
				  regex rx("^[\n\t\r ]*signal[\n\t\r ]*",regex::icase);

				  def_parts[0] = regex_replace(def_parts[0],rx,empty_str);
			  }
		  }

		  vector<string> sig_names = explode(",",def_parts[0]);

		  const string type_def = trim(def_parts[1]);

		  
		  regex rx("^[ \r\n\t]*([^ \r\n\t]*)[ \r\n\t]*(.*)",regex::icase);
		  smatch m;

		  regex_search(type_def.begin(),type_def.end(),m,rx);

		  if( (ptype == 255 && m.size() < 3) || m.size() < 2 )
		  {
			  error_list.push_back("Warning: invalid type definition in line '" + signalstring + "'");
			  return;
		  }
		  string datatypestring;

		  if(ptype == 255 && !without_type)
		  {
			  string ptp = m[1];
			  transform(ptp.begin(),ptp.end(), ptp.begin(), ::tolower); 
			  if(ptp == "in")
				  ptype = P_INPUT;
			  if(ptp == "out")
				  ptype = P_OUTPUT;
			  if(ptp == "inout" || ptp == "buffer")
				  ptype = P_INOUT;

			  datatypestring = m[2]; 
		  }
		  else
		  {
			  datatypestring = type_def; 
		  }

		  if(ptype == 255 && !without_type)
		  {
			  error_list.push_back("Error: undefine signal type in '"+signalstring+"'");
			  return;
		  }

		  dtype dtp = ParseDataType(datatypestring);

		  if(dtp.count == 0)
			  return;
		  

		  for(int i=0; i<sig_names.size();i++)
		  {
			  port out;

			  out.name = trim(sig_names[i]);
			  out.ptype = ptype;
			  out.dtp = dtp;

			  crport.push_back(out);
		  }

	  }

	  /*
	  *		Проверка окончания определения порта
	  */
	  bool portExit(string cur_token)
	  {
		  size_t open_s = count(cur_token.begin(),cur_token.end(),'(');
		  size_t close_s = count(cur_token.begin(),cur_token.end(),')');
		  if(close_s > open_s)
			  return true;

		  return false;
	  }

	  /*
	  *		Парсер типа данных из строки сигнала
	  */
	  dtype ParseDataType(string dt_string)
	  {
		  dt_string = trim(dt_string);

		  string fw = getFirstWord(dt_string);

		  if(fw.find("(") != string::npos)
		  {
			  fw = trim(explode("(",fw)[0]);
		  }

		  dtype out;

		  string tname = getTypeName(fw);

		  if(tname == "array")
		  {
			  vector<int> range= ParseRange(dt_string);
			  out.name = "array";
			  out.count = range[1] - range[0] + 1;
			  out.offset = range[0];

			  string empty_str="";
			  regex rx("^[\n\t\r ]*array[\n\t\r ]*\\([^\\)]*\\)[\n\t\r ]*of[\n\t\r ]*",regex::icase);

			  const string c_dt_string = dt_string;
			  smatch tmp;
			  if(!regex_search(c_dt_string.begin(),c_dt_string.end(),tmp,rx))
			  {
				  error_list.push_back("Error: wrong array defenition in '"+dt_string+"'");

				  out.count = 0;

				  return out;
			  }
		      string new_dt_string = regex_replace(dt_string,rx,empty_str);

			  dtype* atype= new dtype(ParseDataType(new_dt_string));

			  if(atype->count == 0)
				  out.count = 0;

			  out.arr_type = atype;

			  return out;
		  }

		  if(fw == "bit_vector" ||
			  fw == "std_ulogic_vector" ||
			  fw == "std_logic_vector")
		  {
			  vector<int> range= ParseRange(dt_string);

			  out.name = tname;
			  out.count = range[1] - range[0] + 1;
			  out.offset = range[0];
			  out.arr_type = NULL;

			  return out;
		  }

		  out.name = tname;
		  out.count = 1;
		  out.offset = 0;
		  out.arr_type = NULL;

		  return out;
	  }

	  /*
	  *		Парсер интервала для массивов и векторов сигналов
	  */
	  vector<int> ParseRange(const string dt_string)
	  {
		  vector<int> out;

		  regex rx("\\([^\\)0-9]*([0-9]*)[^\\)]*([ \n\r\t]to[ \n\r\t]|[ \n\r\t]downto[ \n\r\t])[^0-9]*([0-9]*)[^\\)]*\\)",regex::icase);
		  smatch m;

		  regex_search(dt_string.begin(),dt_string.end(),m,rx);
		  
		  if(m.size() < 4)
		  {
			  out.push_back(0);
			  out.push_back(0);
			  error_list.push_back("Error: incorrect array range in '"+dt_string+"'");
		  }
		  else
		  {
			  int a=stoi(m[1]);
			  int b=stoi(m[3]);
			  if(a == b)
			  {
				  error_list.push_back("Error: incorrect array range in '"+dt_string+"'");
			  }
			  else
			  if(b < a)
			  {
				out.push_back(b);
				out.push_back(a);
			  }
			  else
			  {
				  out.push_back(a);
				out.push_back(b);
			  }

		  }


		  return out;
	  }

	  /*
	  *		Определение типа данных
	  */
	  string getTypeName(string vhdlname)
	  {
		  transform(vhdlname.begin(),vhdlname.end(), vhdlname.begin(), ::tolower); 

		  if(vhdlname == "std_logic")
		  {
			  return "logic";
		  }
		  if(vhdlname == "bit")
		  {
			  return "bit";
		  }
		  if(vhdlname == "bit_vector")
		  {
			  return "bit";
		  }
		  if(vhdlname == "std_ulogic")
		  {
			  return "ulogic";
		  }
		  if(vhdlname == "std_ulogic_vector")
		  {
			  return "ulogic";
		  }

		  if(vhdlname == "boolean")
		  {
			  return "bool";
		  }

		  if(vhdlname == "integer")
		  {
			  return "int";
		  }

		  if(vhdlname == "float")
		  {
			  return "float";
		  }
		  if(vhdlname == "std_logic_vector")
		  {
			  return "logic";
		  }

		  if(vhdlname == "array")
		  {
			  return "array";
		  }

		  strct tmp;
		  if(!FindUserType(vhdlname,tmp))
			  error_list.push_back("Notice: signal type '"+vhdlname+"' not found");

		  return vhdlname;
	  }

	  /*
	  *		Поиск пользовательского типа данных с определенным названием
	  */
	  bool FindUserType(string name, strct& out_strct)
	  {
		  if(structs.size() == 0)
			  return false;

		  for(int i=0; i<structs.size(); i++)
		  {
			  if(structs[i].name == name)
			  {
				  out_strct = structs[i];
				  return true;
			  }
		  }

		  return false;
	  }

	  /*
	  *		Проверка выхода из определения интерфейса модуля
	  */
	  bool CheckCorrectEnd(const string token, string name)
	  {
		  regex rx("^[ \r\n\t]*end[ \r\n\t]*"+name,regex::icase);
		  smatch m;

		  return regex_search(token.begin(),token.end(),m,rx) ? true : false;
	  }

	  /*
	  *		Получение первой последовательности не пустых символов в строке
	  */
	  string getFirstWord(const string str)
	  {
		  regex rx("^[ \r\n\t]*([^ \r\n\t]*)");
		  smatch m;

		  regex_search(str.begin(),str.end(),m,rx);

		  if(m.size() >= 2)
			  return m[1];
		  else
			  return "";
	  }

	  /*
	  *		Определение типа токена внутри интерфейса модели
	  */
	  int detectArchConstr(const string token)
	  {
		  string first_word = getFirstWord(token);
		  transform(first_word.begin(),first_word.end(), first_word.begin(), ::tolower);

		  if(first_word.find("(") != string::npos)
		  {
			  first_word = trim(explode("(",first_word)[0]);
		  }

		  if(first_word == "port")
			  return ARCH_PORT_CONSTR;
		  if(first_word == "end")
			  return ARCH_END_CONSTR;

		  return UNDEF_CONSTR;
	  }

	  /*
	  *		Определение типа токена внемнего цикла
	  */
	  int detectConstruct(const string token)
	  {
		  string first_word = getFirstWord(token);
		  transform(first_word.begin(),first_word.end(), first_word.begin(), ::tolower);

		  if(first_word == "type")
			  return TYPE_CONSTR;
		  if(first_word == "entity")
			  return MODEL_ARCH_CONSTR;
		  if(first_word == "architecture")
			  return MODEL_IMPL_CONSTR;

		  return UNDEF_CONSTR;
	  }

	  /*
	  *		Разбиение строки на подстроки разделенные делиметром
	  */
	  vector<string> explode(string delimetr, string target)
	  {
		  vector<string> out;
		  string current_part = target;
		  int pos=0;

		  do{
			  pos = current_part.find(delimetr);
			  if( pos != string::npos)
			  {
				  out.push_back(current_part.substr(0,pos));
				  current_part = current_part.substr(pos+delimetr.length(),current_part.length());
			  }
			  else
			  {
				  out.push_back(current_part);
				  break;
			  }
		  }while(true);

		  return out;
	  }

	  /*
	  *		Получение следующего токена из входного потока
	  */
	  bool getToken(string& token)
	  {
		  string tmp;
		  if(tokenlist.size() > 0)
		  {
			  token = getTokenFromExplodeList();
			  return true;

		  }

		  bool out = true;
		  string pre_str = "";
		  while(out = GetString(tmp))
		  {
			  vector<string> tmpp = explode(";",tmp);
			  tmpp[0] = pre_str + " " + tmpp[0];
			  pre_str = tmpp[0];
			  if(tmp.find(";")!=string::npos)
			  {
				  tokenlist = tmpp;

				  ClearTokenList();

				  break;
			  }
		  }
		  if(tokenlist.size() > 0)
		  token = getTokenFromExplodeList();

		  return out;
	  }

	  /*
	  *		Получение следующей строки из входного потока
	  */
	  bool GetString(string& s)
	  {
		  string out="";
		  bool not_end=true;

		  switch(this->mode){
			  case 0:
				  while(!this->file.eof() && out=="")
				  {
					getline(file,out);
					RemoveComments(out);
					out = trim(out);
				  }
				  if(this->file.eof() && out=="")
				  {
					  not_end = false;
				  }

				  break;

			  default:
				  if(!string_readed)
				  {
					  stringlist = explode("\n",this->str);
					  string_readed = true;
				  }
				  if(stringlist.size()==0)
				  {
					  not_end = false;
				  }
				  else
				  {
					  while(stringlist.size()!=0 && out=="")
					  {
						  out = stringlist[0];
						  stringlist.erase(stringlist.begin());

						  RemoveComments(out);
						  out = trim(out);
					  }
					  if(stringlist.size()==0 && out=="")
					  {
						  not_end = false;
					  }
				  }
		  }

		  s= out;
		  return not_end;
	  }

	  /*
	  *		Разбиение токена в случае обноружения исключения
	  */
	  string getTokenFromExplodeList()
	  {
		  string delimetr_list[]={"is","begin"};
		  string first = tokenlist[0];
		  tokenlist.erase(tokenlist.begin());

		  smatch match;
		  regex rx;

		  for(int i =0; i<2; i++)
		  {
			  string del=delimetr_list[i];
			  const string text = first;

			  rx=regex("([\n\t\r ]+|^)"+del+"([\n\t\r ]+|$)",regex::icase);
			  if(regex_search(text.begin(),text.end(),match,rx))
			  {
				  vector<string> exp_list = explode(match[0],first);
				  tokenlist.insert(tokenlist.begin(),exp_list.begin(),exp_list.end());
				  ClearTokenList();

				  first = tokenlist[0];
				  tokenlist.erase(tokenlist.begin());
			  }
		  }

		  return first;
	  }

	  /*
	  *		Удаление из начала и конца строки символов переноса и пробелов
	  */
	  string trim(string s)
	  {
		  string empty_str="";
		  regex rx("(^[\n\t\r ]*)|([\n\t\r ]*$)");

		  return regex_replace(s,rx,empty_str);
	  }

	  /*
	  *		Удаление пустых строк из списка токенов
	  */
	  void ClearTokenList()
	  {
		  for(int i=0; i<tokenlist.size(); i++)
		  {
			  tokenlist[i] = trim(tokenlist[i]);

			  if(tokenlist[i] == "")
				  tokenlist.erase(tokenlist.begin()+i);
		  }
	  }

	  /*
	  *		Удаление комментариев из строки
	  */
	  void RemoveComments(string& str)
	  {
		  string empty_str="";
		  regex rx("--.*$");

		  str = regex_replace(str,rx,empty_str);
	  }
};