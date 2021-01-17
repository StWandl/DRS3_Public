#ifndef RPIUNDERTEST_H
#define RPIUNDERTEST_H

class RPIUnderTest
{
public:
	RPIUnderTest() = delete;
	RPIUnderTest(int const& systemNumber, int const& colWidth);

	void updatePins();

private:
	int mColWidth;
	int mSystemNumber;

	void initPins();
	void printInfo(std::string const& masterClientInfo, std::string const& connectionInfo, std::string const& sendReceiveInfo);
};

#endif