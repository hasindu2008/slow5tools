#include <inttypes.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <math.h> // TODO need this?
#include "unit_test.h"
#include "slow5.h"
#include "slow5_extra.h"
#include "misc.h"

char double_overflow[] = "11189731495357231765021263853030970205169063322294624200440323733891737005522970722616410290336528882853545697807495577314427443153670288434198125573853743678673593200706973263201915918282961524365529510646791086614311790632169778838896134786560600399148753433211454911160088679845154866512852340149773037600009125479393966223151383622417838542743917838138717805889487540575168226347659235576974805113725649020884855222494791399377585026011773549180099796226026859508558883608159846900235645132346594476384939859276456284579661772930407806609229102715046085388087959327781622986827547830768080040150694942303411728957777100335714010559775242124057347007386251660110828379119623008469277200965153500208474470792443848545912886723000619085126472111951361467527633519562927597957250278002980795904193139603021470997035276467445530922022679656280991498232083329641241038509239184734786121921697210543484287048353408113042573002216421348917347174234800714880751002064390517234247656004721768096486107994943415703476320643558624207443504424380566136017608837478165389027809576975977286860071487028287955567141404632615832623602762896316173978484254486860609948270867968048078702511858930838546584223040908805996294594586201903766048446790926002225410530775901065760671347200125846406957030257138960983757998926954553052368560758683179223113639519468850880771872104705203957587480013143131444254943919940175753169339392366881856189129931729104252921236835159922322050998001677102784035360140829296398115122877768135706045789343535451696539561254048846447169786893211671087229088082778350518228857646062218739702851655083720992349483334435228984751232753726636066213902281264706234075352071724058665079518217303463782631353393706774901950197841690441824738063162828586857741432581165364040218402724913393320949219498422442730427019873044536620350262386957804682003601447291997123095530057206141866974852846856186514832715974481203121946751686379343096189615107330065552421485195201762858595091051839472502863871632494167613804996319791441870254302706758495192008837915169401581740046711477877201459644461175204059453504764721807975761111720846273639279600339670470037613374509553184150073796412605047923251661354841291884211340823015473304754067072818763503617332908005951896325207071673904547777129682265206225651439919376804400292380903112437912614776255964694221981375146967079446870358004392507659451618379811859392049544036114915310782251072691486979809240946772142727012404377187409216756613634938900451232351668146089322400697993176017805338191849981933008410985993938760292601390911414526003720284872132411955424282101831204216104467404621635336900583664606591156298764745525068145003932941404131495400677602951005962253022823003631473824681059648442441324864573137437595096416168048024129351876204668135636877532814675538798871771836512893947195335061885003267607354388673368002074387849657014576090349857571243045102038730494854256702479339322809110526041538528994849203991091946129912491633289917998094380337879522093131466946149705939664152375949285890960489916121944989986384837022486672249148924678410206183364627416969576307632480235587975245253737035433882960862753427740016333434055083537048507374544819754722228975281083020898682633020285259923084168054539687911418297629988964576482765287504562854924265165217750799516259669229114977788962356670956627138482018191348321687995863652637620978285070099337294396784639879024914514222742527006363942327998483976739987154418554201562244154926653014515504685489258620276085761837129763358761215382565129633538141663949516556000264159186554850057052611431952919918807954522394649627635630178580896692226406235382898535867595990647008385687123810329591926494846250768992258419305480763620215089022149220528069842018350840586938493815498909445461977893029113576516775406232278298314033473276603952231603422824717528181818844304880921321933550869873395861276073670866652375555675803171490108477320096424318780070008797346032906278943553743564448851907191616455141155761939399690767415156402826543664026760095087523945507341556135867933066031744720924446513532366647649735400851967040771103640538150073486891798364049570606189535005089840913826869535090066783324472578712196604415284924840041850932811908963634175739897166596000759487800619164094854338758520657116541072260996288150123144377944008749301944744330784388995701842710004808305012177123560622895076269042856800047718893158089358515593863176652948089031267747029662545110861548958395087796755464137944895960527975209874813839762578592105756284401759349324162148339565350189196811389091843795734703269406342890087805846940352453479398080674273236297887100867175802531561302356064878709259865288416350972529537091114317204887747405539054009425375424119317944175137064689643861517718849867010341532542385911089624710885385808688837777258648564145934262121086647588489260031762345960769508849149662444156604419552086811989770240.000000";

int ato_xintx_valid(void) {
    int err;
    char buf[256];

    ASSERT(ato_uint8("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_uint8("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_uint8("100", &err) == 100);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu32, UINT8_MAX);
    ASSERT(ato_uint8(buf, &err) == UINT8_MAX);
    ASSERT(err == 0);

    ASSERT(ato_uint32("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_uint32("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_uint32("100", &err) == 100);
    ASSERT(err == 0);
    ASSERT(ato_uint32("2000", &err) == 2000);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu32, UINT32_MAX);
    ASSERT(ato_uint32(buf, &err) == UINT32_MAX);
    ASSERT(err == 0);

    ASSERT(ato_uint64("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_uint64("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_uint64("100", &err) == 100);
    ASSERT(err == 0);
    ASSERT(ato_uint64("2000", &err) == 2000);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu32, UINT32_MAX);
    ASSERT(ato_uint64(buf, &err) == UINT32_MAX);
    ASSERT(err == 0);
    sprintf(buf, "%" PRId64, INT64_MAX);
    ASSERT(ato_uint64(buf, &err) == INT64_MAX);
    ASSERT(err == 0);
    sprintf(buf, "%" PRIu64, UINT64_MAX);
    ASSERT(ato_uint64(buf, &err) == UINT64_MAX);
    ASSERT(err == 0);

    sprintf(buf, "%" PRId16, (int16_t) INT16_MIN);
    ASSERT(ato_int16(buf, &err) == (int16_t) INT16_MIN);
    ASSERT(err == 0);
    ASSERT(ato_int16("-2000", &err) == -2000);
    ASSERT(err == 0);
    ASSERT(ato_int16("-100", &err) == -100);
    ASSERT(err == 0);
    ASSERT(ato_int16("-1", &err) == -1);
    ASSERT(err == 0);
    ASSERT(ato_int16("0", &err) == 0);
    ASSERT(err == 0);
    ASSERT(ato_int16("1", &err) == 1);
    ASSERT(err == 0);
    ASSERT(ato_int16("100", &err) == 100);
    ASSERT(err == 0);
    ASSERT(ato_int16("2000", &err) == 2000);
    ASSERT(err == 0);
    sprintf(buf, "%" PRId16, (int16_t) INT16_MAX);
    ASSERT(ato_int16(buf, &err) == (int16_t) INT16_MAX);
    ASSERT(err == 0);

    return EXIT_SUCCESS;
}

int ato_xintx_invalid(void) {
    int err;
    char buf[256];

    ASSERT(ato_uint8("-1", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint8("-100", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint8("-1000", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint8("-lol", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu16, (uint16_t) ( (uint16_t) UINT8_MAX + 1));
    ASSERT(ato_uint8(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu16, (uint16_t) UINT16_MAX);
    ASSERT(ato_uint8(buf, &err) == 0);
    ASSERT(err == -1);

    ASSERT(ato_uint32("-1", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint32("-100", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint32("-1000", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu64, (uint64_t) (UINT32_MAX) + 1);
    ASSERT(ato_uint32(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRIu64, UINT64_MAX);
    ASSERT(ato_uint32(buf, &err) == 0);
    ASSERT(err == -1);

    ASSERT(ato_uint64("-1", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint64("-100", &err) == 0);
    ASSERT(err == -1);
    ASSERT(ato_uint64("-1000", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId64, INT64_MIN);
    ASSERT(ato_uint64(buf, &err) == 0);
    ASSERT(err == -1);

    sprintf(buf, "%" PRIu16, (uint16_t) UINT16_MAX);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId32, (int32_t) (INT16_MIN) - 1);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId32, (int32_t) (INT16_MAX) + 1);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%" PRId64, INT64_MIN);
    ASSERT(ato_int16(buf, &err) == 0);
    ASSERT(err == -1);

    return EXIT_SUCCESS;
}

int strtod_check_valid(void) {
    int err;
    char buf[512];

    errno=0;

    ASSERT(strtod_check("0", &err) == (double) 0);
    ASSERT(err == 0);
    ASSERT(strtod_check("1", &err) == (double) 1);
    ASSERT(err == 0);
    ASSERT(strtod_check("100", &err) == (double) 100);
    ASSERT(err == 0);
    ASSERT(strtod_check("-100", &err) == (double) -100);
    ASSERT(err == 0);
    ASSERT(strtod_check("0.0", &err) == (double) 0.0);
    ASSERT(err == 0);
    ASSERT(strtod_check("2.5", &err) == (double) 2.5);
    ASSERT(err == 0);
    ASSERT(strtod_check("-100.7892", &err) == (double) -100.7892);
    ASSERT(err == 0);
    ASSERT(strtod_check("1.2", &err) == (double) 1.2);
    ASSERT(err == 0);
    sprintf(buf, "%lf", -DBL_MAX);
    ASSERT(strtod_check(buf, &err) == -DBL_MAX);
    ASSERT(err == 0);
    sprintf(buf, "%lf", DBL_MIN);
    ASSERT(approx(strtod_check(buf, &err), DBL_MIN));
    ASSERT(err == 0);
    sprintf(buf, "%lf", DBL_MAX);
    ASSERT(strtod_check(buf, &err) == DBL_MAX);
    ASSERT(err == 0);

    return EXIT_SUCCESS;
}

int strtod_check_invalid(void) {
    int err;
    char buf[8192];

    ASSERT(strtod_check("hithere", &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("sometext", &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("inf", &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("-inf", &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%le", 10e3);
    ASSERT(strtod_check(buf, &err) == 0);
    ASSERT(err == -1);
    sprintf(buf, "%le", -10e-3);
    ASSERT(strtod_check(buf, &err) == 0);
    ASSERT(err == -1);
    ASSERT(strtod_check("-inf", &err) == 0);
    ASSERT(err == -1);

    strcpy(buf,double_overflow);

    //sprintf(buf, "%Lf", LDBL_MAX);
    //if (strcmp(buf, "inf") != 0) { // TODO why does valgrind give "inf"?
    ASSERT(strtod_check(buf, &err) == HUGE_VAL);
    ASSERT(err == -1);
    //}
    //sprintf(buf, "%Lf", -LDBL_MAX);
    strcpy(buf+1,double_overflow);
    buf[0]='-';
    //if (strcmp(buf, "-inf") != 0) {
    ASSERT(strtod_check(buf, &err) == -HUGE_VAL);
    ASSERT(err == -1);
    //}

    return EXIT_SUCCESS;
}

int has_fast5_ext_valid(void) {

    ASSERT(has_fast5_ext("test.fast5"));
    ASSERT(has_fast5_ext("hithere/test.fast5"));
    ASSERT(has_fast5_ext("testaskdj.fast5"));
    ASSERT(has_fast5_ext("fast5.fast5"));
    ASSERT(has_fast5_ext("slow5.fast5"));
    ASSERT(has_fast5_ext("hi...fast5.fast5"));
    ASSERT(has_fast5_ext("1234.fast5"));
    ASSERT(has_fast5_ext("myfast5.fast5"));
    ASSERT(has_fast5_ext("hithere///test.fast5"));
    ASSERT(has_fast5_ext("hithere///test.fast5/test.fast5"));
    ASSERT(has_fast5_ext("testaslkdjlaskjdfalsdifaslkfdj234.fast5"));
    ASSERT(has_fast5_ext(".fast5"));

    return EXIT_SUCCESS;
}

int has_fast5_ext_invalid(void) {

    ASSERT(!has_fast5_ext("."));
    ASSERT(!has_fast5_ext("..."));
    ASSERT(!has_fast5_ext("fast5"));
    ASSERT(!has_fast5_ext("fast5."));
    ASSERT(!has_fast5_ext("blow5"));
    ASSERT(!has_fast5_ext("blablabla"));
    ASSERT(!has_fast5_ext(""));
    ASSERT(!has_fast5_ext(NULL));

    return EXIT_SUCCESS;
}

int is_dir_valid(void) {

    ASSERT(is_dir("/"));
    ASSERT(is_dir("////////"));
    ASSERT(is_dir("."));
    ASSERT(is_dir("./"));
    ASSERT(is_dir(".."));
    ASSERT(is_dir("../"));
    ASSERT(is_dir("../test"));
    ASSERT(is_dir("../test/"));
    ASSERT(is_dir("data///"));
    ASSERT(is_dir("../src"));
    ASSERT(is_dir("random"));
    ASSERT(is_dir("data/exp/one_fast5"));

    return EXIT_SUCCESS;
}

int is_dir_invalid(void) {

    ASSERT(!is_dir("./unit_test"));
    ASSERT(!is_dir("notadir"));
    ASSERT(!is_dir("lolwhat"));
    ASSERT(!is_dir("///lolwhat///"));
    ASSERT(!is_dir("./unit_test.c"));
    ASSERT(!is_dir(""));
    ASSERT(!is_dir("../Make"));
    ASSERT(!is_dir("../Makefile"));
    ASSERT(!is_dir(NULL));

    return EXIT_SUCCESS;
}

int path_get_slow5_fmt_test(void) {

    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("test.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("hithere/test.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("testaskdj.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("blow5.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("slow5.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("hi...slow5.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("1234.slow5"));
    ASSERT(FORMAT_ASCII == path_get_slow5_fmt("myslow5.slow5"));

    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("test.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("hithere///test.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("hithere///test.slow5/test.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("blow5.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("slow5.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("hi...blow5.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("1234.blow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt("myblow5.blow5"));

    ASSERT(FORMAT_ASCII == path_get_slow5_fmt(".slow5"));
    ASSERT(FORMAT_BINARY == path_get_slow5_fmt(".blow5"));

    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("..."));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("slow5"));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("blow5"));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt("blablabla"));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt(""));
    ASSERT(FORMAT_UNKNOWN == path_get_slow5_fmt(NULL));

    return EXIT_SUCCESS;
}

int name_get_slow5_fmt_test(void) {

    ASSERT(FORMAT_ASCII == name_get_slow5_fmt("slow5"));
    ASSERT(FORMAT_BINARY == name_get_slow5_fmt("blow5"));

    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(".slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(".blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(".blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("..."));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("slow55"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blow55"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blablabla"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(""));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt(NULL));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("test.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere/test.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("testaskdj.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("slow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hi...slow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("1234.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("myslow5.slow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("test.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere///test.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hithere///test.slow5/test.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("testaslkdjlaskjdfalsdifaslkfdj234.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("blow5.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("slow5.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("hi...blow5.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("1234.blow5"));
    ASSERT(FORMAT_UNKNOWN == name_get_slow5_fmt("myblow5.blow5"));

    return EXIT_SUCCESS;
}

int asprintf_mine_valid(void) {
    char *str = NULL;

    double x = 225.69;
    const char *exp = "225.690000";
    ASSERT(asprintf_mine(&str, "%f", x) == strlen(exp));
    ASSERT(strcmp(str, exp) == 0);

    free(str);

    x = 225.691234;
    exp = "225.691234";
    ASSERT(asprintf_mine(&str, "%f", x) == strlen(exp));
    ASSERT(strcmp(str, exp) == 0);

    free(str);

    x = 225.6912345;
    exp = "225.691235"; // Note that exp is different to x here
    ASSERT(asprintf_mine(&str, "%f", x) == strlen(exp));
    ASSERT(strcmp(str, exp) == 0);

    free(str);

    x = 225.69123456789;
    exp = "225.691235"; // Note that exp is different to x here
    ASSERT(asprintf_mine(&str, "%f", x) == strlen(exp));
    ASSERT(strcmp(str, exp) == 0);

    free(str);

    return EXIT_SUCCESS;
}


int main(void) {

    struct command tests[] = {
        // Helpers
        CMD(ato_xintx_valid)
        CMD(ato_xintx_invalid)

        CMD(strtod_check_valid)
        CMD(strtod_check_invalid)

        CMD(has_fast5_ext_valid)
        CMD(has_fast5_ext_invalid)

        CMD(has_fast5_ext_valid)
        CMD(has_fast5_ext_invalid)

        CMD(path_get_slow5_fmt_test)
        CMD(name_get_slow5_fmt_test)

        CMD(asprintf_mine_valid)
    };

    return RUN_TESTS(tests);
}
