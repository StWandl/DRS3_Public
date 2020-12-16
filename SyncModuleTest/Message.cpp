// Stefan Wandl

#include "Message.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

#include <memory>
#include <iostream>

using namespace rapidjson;
using namespace std;

namespace Priv
{
	template<typename Writer>
	void toString(Writer& writer, const Message * msg)
	{
		writer.StartObject();
		writer.Key("id");
		writer.String(msg->id().c_str());
		writer.Key("timestamp");
		writer.String(msg->timestamp().c_str());

		writer.Key("node");
		writer.StartObject();
		writer.Key("address");
		writer.String(msg->address().c_str());
		writer.Key("priority");
		writer.Int(msg->priority());
		writer.EndObject();

		writer.Key("msg");
		writer.StartObject();
		writer.Key("type");
		writer.Int(msg->type());
		writer.Key("payload");
		writer.String(msg->payload().c_str());
		writer.EndObject();

		writer.EndObject();
	}

	enum class ParserState
	{
		INVALID,
		INIT,
		ID,
		TIMESTAMP,
		NODE,
		NODE_START,
		ADDRESS,
		PRIORITY,
		NODE_END,
		MSG,
		MSG_START,
		TYPE,
		PAYLOAD,
		MSG_END,
		DONE
	};

	struct MsgParser
	{
		using State = ParserState;

		MsgParser(Message& msg)
			: m_msg(msg)
			, m_state(State::INVALID)
		{}

		bool Null()
		{
			cout << "Null()" << endl;
			return true;
		}

		bool Bool(bool b)
		{
			cout << "Bool(" << boolalpha << b << ")" << endl;
			return true;
		}
		
		bool Double(double d)
		{
			cout << "Double(" << d << ")" << endl;
			return true;
		}

		bool RawNumber(const char* str, size_t length, bool copy)
		{
			cout << "Number(" << str << ", " << length << ", " << boolalpha << copy << ")" << endl;
			return true;
		}

		bool Int(int i)
		{ 
			return Int64(i);
		}

		bool Uint(unsigned u)
		{ 
			return Int64(u);
		}
		
		bool Uint64(uint64_t u)
		{ 
			return Int64(u);
		}

		bool Int64(int64_t i)
		{
			if (m_state == State::PRIORITY)
			{
				m_msg.setPriority(i);
			}
			else if (m_state == State::TYPE)
			{
				m_msg.setType(i);
			}
			else
			{
				cout << "Unexpected position of integer value: " << i << endl;
				return false;
			}

			return true;
		}

		bool String(const char* str, size_t length, bool copy)
		{
			if (m_state == State::ID)
			{
				m_msg.setId(str);
			}
			else if (m_state == State::TIMESTAMP)
			{
				m_msg.setTimestamp(str);
			}
			else if (m_state == State::ADDRESS)
			{
				m_msg.setAddress(str);
			}
			else if (m_state == State::PAYLOAD)
			{
				m_msg.setPayload(str);
			}
			else
			{
				cout << "Unexpected position of string value: " << str << endl;
				return false;
			}

			return true;
		}
		bool StartObject()
		{ 
			if (m_state == State::INVALID)
				m_state = State::INIT;
			else if (m_state == State::NODE)
				m_state = State::NODE_START;
			else if (m_state == State::MSG)
				m_state = State::MSG_START;
			else
			{
				cout << "Unexpected position of StartObject '{' found!" << endl;
				return false;
			}

			return true;
		}
		bool Key(const char* str, size_t length, bool copy)
		{
			string key(str);

			if (key == "id")
			{
				if(m_state == State::INIT)
					m_state = State::ID;
				else
				{
					cout << "Unexpected position of \"id\" key" << endl;
					return false;
				}
			}
			else if (key == "timestamp")
			{
				if (m_state == State::ID)
					m_state = State::TIMESTAMP;
				else
				{
					cout << "Unexpected position of \"timestamp\" key" << endl;
					return false;
				}
			}
			else if (key == "node")
			{
				if (m_state == State::TIMESTAMP)
					m_state = State::NODE;
				else
				{
					cout << "Unexpected position of \"node\" key" << endl;
					return false;
				}
			}
			else if (key == "address")
			{
				if (m_state == State::NODE_START)
					m_state = State::ADDRESS;
				else
				{
					cout << "Unexpected position of \"address\" key" << endl;
					return false;
				}
			}
			else if (key == "priority")
			{
				if (m_state == State::ADDRESS)
					m_state = State::PRIORITY;
				else
				{
					cout << "Unexpected position of \"priority\" key" << endl;
					return false;
				}
			}
			else if (key == "msg")
			{
				if (m_state == State::NODE_END)
					m_state = State::MSG;
				else
				{
					cout << "Unexpected position of \"msg\" key" << endl;
					return false;
				}
			}
			else if (key == "type")
			{
				if (m_state == State::MSG_START)
					m_state = State::TYPE;
				else
				{
					cout << "Unexpected position of \"type\" key" << endl;
					return false;
				}
			}
			else if (key == "payload")
			{
				if (m_state == State::TYPE)
					m_state = State::PAYLOAD;
				else
				{
					cout << "Unexpected position of \"payload\" key" << endl;
					return false;
				}
			}
			else
			{
				cout << "Unexpected Key found!" << endl;
				return false;
			}

			return true;
		}
		bool EndObject(size_t memberCount)
		{ 
			if (m_state == State::PRIORITY)
				m_state = State::NODE_END;
			else if (m_state == State::PAYLOAD)
				m_state = State::MSG_END;
			else if (m_state == State::MSG_END)
			{
				m_state = State::DONE;
				//m_msg.setIsValid(true);
			}
			else
			{
				cout << "Unexpected position of EndObject '}' found!" << endl;
				return false;
			}
			return true;
		}

		bool StartArray() { cout << "StartArray()" << endl; return true; }
		bool EndArray(size_t elementCount) { cout << "EndArray(" << elementCount << ")" << endl; return true; }

	private:
		Message& m_msg;
		State m_state;
	};
}

Message::Message()
	: m_id("")
	, m_timestamp("")
	, m_address("")
	, m_priority(-1)
	, m_type(-1)
	, m_payload("")
	, m_isValid(false)
{
}

Message::Message(const std::string& id, const std::string& timestamp, const std::string& address, int priority, int type, const std::string& payload)
	: m_id(id)
	, m_timestamp(timestamp)
	, m_address(address)
	, m_priority(priority)
	, m_type(type)
	, m_payload(payload)
{
	updateIsValid();
}

std::string Message::toString(bool prettyFormat) const
{
	StringBuffer buf;

	if (prettyFormat)
	{
		PrettyWriter<StringBuffer> writer(buf);
		Priv::toString(writer, this);
	}
	else
	{
		Writer<StringBuffer> writer(buf);
		Priv::toString(writer, this);
	}

	return buf.GetString();
}

bool Message::isValid() const
{
	return m_isValid;
}

Message Message::fromString(const std::string& str)
{
	Message newMsg;
	
	Priv::MsgParser parser(newMsg);
	Reader reader;
	StringStream ss(str.c_str());
	reader.Parse(ss, parser);

	return newMsg;
}

const std::string& Message::id() const
{
	return m_id;
}

void Message::setId(const std::string& id)
{
	m_id = id;
	updateIsValid();
}

const std::string& Message::timestamp() const
{
	return m_timestamp;
}

void Message::setTimestamp(const std::string& timestamp)
{
	m_timestamp = timestamp;
	updateIsValid();
}

const std::string& Message::address() const
{
	return m_address;
}

void Message::setAddress(const std::string& address)
{
	m_address = address;
	updateIsValid();
}

int Message::priority() const
{
	return m_priority;
}

void Message::setPriority(int priority)
{
	m_priority = priority;
	updateIsValid();
}

int Message::type() const
{
	return m_type;
}

void Message::setType(int type)
{
	m_type = type;
	updateIsValid();
}

const std::string& Message::payload() const
{
	return m_payload;
}

void Message::setPayload(const std::string& payload)
{
	m_payload = payload;
	updateIsValid();
}

void Message::updateIsValid()
{
	m_isValid = 
		!m_id.empty() && 
		!m_timestamp.empty() && 
		!m_address.empty() && 
		m_priority > 0 && 
		m_type > 0 && m_type < 8 && 
		((m_type == 6 || m_type == 7) ? !m_payload.empty() : true);
}
