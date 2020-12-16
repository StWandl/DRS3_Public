 #ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

enum MsgType
{
	SlaveRequest = 1,
	MasterACK = 2,
	NewMaster = 3,
	NewMasterACK = 4,
	RequestTimestamp = 5,
	Timestamp = 6,
	Deviation = 7
};

class Message
{
public:
	Message();
	Message(const std::string& id, const std::string& timestamp, const std::string& address, int priority, int type, const std::string& payload);

	std::string toString(bool prettyFormat = false) const;

	bool isValid() const;

	static Message fromString(const std::string& str);

	const std::string& id() const;
	void setId(const std::string& id);

	const std::string& timestamp() const;
	void setTimestamp(const std::string& timestamp);

	const std::string& address() const;
	void setAddress(const std::string& address);

	int priority() const;
	void setPriority(int priority);

	int type() const;
	void setType(int type);

	const std::string& payload() const;
	void setPayload(const std::string& payload);

private:
	bool m_isValid = false;
	std::string m_id;
	std::string m_timestamp;
	std::string m_address;
	int m_priority;
	int m_type;
	std::string m_payload;

	void updateIsValid();
};


#endif

