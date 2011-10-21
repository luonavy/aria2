#include "OptionParser.h"

#include <cstring>
#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include "OptionHandlerImpl.h"
#include "Exception.h"
#include "util.h"
#include "Option.h"
#include "array_fun.h"
#include "prefs.h"

namespace aria2 {

class OptionParserTest:public CppUnit::TestFixture {

  CPPUNIT_TEST_SUITE(OptionParserTest);
  CPPUNIT_TEST(testFindAll);
  CPPUNIT_TEST(testFindByNameSubstring);
  CPPUNIT_TEST(testFindByTag);
  CPPUNIT_TEST(testFindByName);
  CPPUNIT_TEST(testFindByShortName);
  CPPUNIT_TEST(testFindByID);
  CPPUNIT_TEST(testParseDefaultValues);
  CPPUNIT_TEST(testParseArg);
  CPPUNIT_TEST(testParse);
  CPPUNIT_TEST_SUITE_END();
private:
  SharedHandle<OptionParser> oparser_;
public:
  void setUp()
  {
    oparser_.reset(new OptionParser());

    SharedHandle<OptionHandler> timeout
      (new DefaultOptionHandler(PREF_TIMEOUT, NO_DESCRIPTION, "ALPHA", "",
                                OptionHandler::REQ_ARG, 'A'));
    timeout->addTag("apple");
    timeout->setEraseAfterParse(true);
    oparser_->addOptionHandler(timeout);

    SharedHandle<OptionHandler> dir(new DefaultOptionHandler(PREF_DIR));
    dir->addTag("apple");
    dir->addTag("orange");
    dir->addTag("pineapple");
    oparser_->addOptionHandler(dir);

    SharedHandle<DefaultOptionHandler> daemon
      (new DefaultOptionHandler(PREF_DAEMON, NO_DESCRIPTION, "CHARLIE", "",
                                OptionHandler::REQ_ARG, 'C'));
    daemon->hide();
    daemon->addTag("pineapple");
    oparser_->addOptionHandler(daemon);

    SharedHandle<OptionHandler> out
      (new UnitNumberOptionHandler(PREF_OUT, NO_DESCRIPTION, "1M",
                                   -1, -1, 'D'));
    out->addTag("pineapple");
    oparser_->addOptionHandler(out);    
  }

  void tearDown() {}

  void testFindAll();
  void testFindByNameSubstring();
  void testFindByTag();
  void testFindByName();
  void testFindByShortName();
  void testFindByID();
  void testParseDefaultValues();
  void testParseArg();
  void testParse();
};


CPPUNIT_TEST_SUITE_REGISTRATION(OptionParserTest);

void OptionParserTest::testFindAll()
{
  std::vector<SharedHandle<OptionHandler> > res = oparser_->findAll();
  CPPUNIT_ASSERT_EQUAL((size_t)3, res.size());
  CPPUNIT_ASSERT_EQUAL(std::string("timeout"), res[0]->getName());
  CPPUNIT_ASSERT_EQUAL(std::string("dir"), res[1]->getName());
  CPPUNIT_ASSERT_EQUAL(std::string("out"), res[2]->getName());
}

void OptionParserTest::testFindByNameSubstring()
{
  std::vector<SharedHandle<OptionHandler> > res =
    oparser_->findByNameSubstring("i");
  CPPUNIT_ASSERT_EQUAL((size_t)2, res.size());
  CPPUNIT_ASSERT_EQUAL(std::string("timeout"), res[0]->getName());
  CPPUNIT_ASSERT_EQUAL(std::string("dir"), res[1]->getName());
}

void OptionParserTest::testFindByTag()
{
  std::vector<SharedHandle<OptionHandler> > res =
    oparser_->findByTag("pineapple");
  CPPUNIT_ASSERT_EQUAL((size_t)2, res.size());
  CPPUNIT_ASSERT_EQUAL(std::string("dir"), res[0]->getName());
  CPPUNIT_ASSERT_EQUAL(std::string("out"), res[1]->getName());
}

void OptionParserTest::testFindByName()
{
  SharedHandle<OptionHandler> dir = oparser_->findByName("dir");
  CPPUNIT_ASSERT(dir);
  CPPUNIT_ASSERT_EQUAL(std::string("dir"), dir->getName());

  SharedHandle<OptionHandler> daemon = oparser_->findByName("daemon");
  CPPUNIT_ASSERT(!daemon);

  SharedHandle<OptionHandler> timeout2 = oparser_->findByName("timeout2");
  CPPUNIT_ASSERT(!timeout2);
}

void OptionParserTest::testFindByShortName()
{
  SharedHandle<OptionHandler> timeout = oparser_->findByShortName('A');
  CPPUNIT_ASSERT(timeout);
  CPPUNIT_ASSERT_EQUAL(std::string("timeout"), timeout->getName());

  CPPUNIT_ASSERT(!oparser_->findByShortName('C'));
}

void OptionParserTest::testFindByID()
{
  SharedHandle<OptionHandler> timeout = oparser_->findByID(1);
  CPPUNIT_ASSERT(timeout);
  CPPUNIT_ASSERT_EQUAL(std::string("timeout"), timeout->getName());

  CPPUNIT_ASSERT(!oparser_->findByID(3));
}

void OptionParserTest::testParseDefaultValues()
{
  Option option;
  oparser_->parseDefaultValues(option);
  CPPUNIT_ASSERT_EQUAL(std::string("ALPHA"), option.get(PREF_TIMEOUT));
  CPPUNIT_ASSERT_EQUAL(std::string("1048576"), option.get(PREF_OUT));
  CPPUNIT_ASSERT_EQUAL(std::string("CHARLIE"), option.get(PREF_DAEMON));
  CPPUNIT_ASSERT(!option.defined(PREF_DIR));
}

void OptionParserTest::testParseArg()
{
  Option option;
  char prog[7];
  strncpy(prog, "aria2c", sizeof(prog));

  char optionTimeout[3];
  strncpy(optionTimeout, "-A", sizeof(optionTimeout));
  char argTimeout[6];
  strncpy(argTimeout, "ALPHA", sizeof(argTimeout));
  char optionDir[8];
  strncpy(optionDir, "--dir", sizeof(optionDir));
  char argDir[6];
  strncpy(argDir, "BRAVO", sizeof(argDir));

  char nonopt1[8];
  strncpy(nonopt1, "nonopt1", sizeof(nonopt1));
  char nonopt2[8];
  strncpy(nonopt2, "nonopt2", sizeof(nonopt2));

  char* argv[] = { prog, optionTimeout, argTimeout, optionDir, argDir,
                   nonopt1, nonopt2 };
  int argc = A2_ARRAY_LEN(argv);

  std::stringstream s;
  std::vector<std::string> nonopts;

  oparser_->parseArg(s, nonopts, argc, argv);

  CPPUNIT_ASSERT_EQUAL(std::string("timeout=ALPHA\n"
                                   "dir=BRAVO\n"), s.str());

  CPPUNIT_ASSERT_EQUAL((size_t)2, nonopts.size());
  CPPUNIT_ASSERT_EQUAL(std::string("nonopt1"), nonopts[0]);
  CPPUNIT_ASSERT_EQUAL(std::string("nonopt2"), nonopts[1]);

  CPPUNIT_ASSERT_EQUAL(std::string("*****"), std::string(argTimeout));
}

void OptionParserTest::testParse()
{
  Option option;
  std::istringstream in("timeout=Hello\n"
                        "UNKNOWN=x\n"
                        "\n"
                        "dir=World");
  oparser_->parse(option, in);
  CPPUNIT_ASSERT_EQUAL(std::string("Hello"), option.get(PREF_TIMEOUT));
  CPPUNIT_ASSERT_EQUAL(std::string("World"), option.get(PREF_DIR));
}

} // namespace aria2
