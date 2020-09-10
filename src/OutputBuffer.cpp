/* Memory buffer for handling output data
   Copyright (C) 2018-2020 Adam Leszczynski (aleszczynski@bersler.com)

This file is part of OpenLogReplicator.

OpenLogReplicator is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 3, or (at your option)
any later version.

OpenLogReplicator is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenLogReplicator; see the file LICENSE;  If not see
<http://www.gnu.org/licenses/>.  */

#include <string.h>

#include "CharacterSet16bit.h"
#include "CharacterSet7bit.h"
#include "CharacterSet8bit.h"
#include "CharacterSetAL16UTF16.h"
#include "CharacterSetAL32UTF8.h"
#include "CharacterSetJA16EUC.h"
#include "CharacterSetJA16EUCTILDE.h"
#include "CharacterSetJA16SJIS.h"
#include "CharacterSetJA16SJISTILDE.h"
#include "CharacterSetKO16KSCCS.h"
#include "CharacterSetUTF8.h"
#include "CharacterSetZHS16GBK.h"
#include "CharacterSetZHS32GB18030.h"
#include "CharacterSetZHT16HKSCS31.h"
#include "CharacterSetZHT32EUC.h"
#include "CharacterSetZHT32TRIS.h"
#include "OutputBuffer.h"
#include "OracleAnalyser.h"
#include "OracleColumn.h"
#include "OracleObject.h"
#include "RedoLogRecord.h"
#include "RuntimeException.h"

namespace OpenLogReplicator {

    const char OutputBuffer::translationMap[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    OutputBuffer::OutputBuffer(uint64_t messageFormat, uint64_t xidFormat, uint64_t timestampFormat, uint64_t charFormat, uint64_t scnFormat,
            uint64_t unknownFormat, uint64_t schemaFormat, uint64_t columnFormat) :
            oracleAnalyser(nullptr),
            messageFormat(messageFormat),
            xidFormat(xidFormat),
            timestampFormat(timestampFormat),
            charFormat(charFormat),
            scnFormat(scnFormat),
            unknownFormat(unknownFormat),
            schemaFormat(schemaFormat),
            columnFormat(columnFormat),
            messageLength(0),
            lastTime(0),
            lastScn(0),
            lastXid(0),
            defaultCharacterMapId(0),
            defaultCharacterNcharMapId(0),
            writer(nullptr),
            buffersAllocated(0),
            firstBufferPos(0),
            firstBuffer(nullptr),
            curBuffer(nullptr),
            curBufferPos(0),
            lastBuffer(nullptr),
            lastBufferPos(0) {

        characterMap[1] = new CharacterSet7bit("US7ASCII", CharacterSet7bit::unicode_map_US7ASCII);
        characterMap[2] = new CharacterSet8bit("WE8DEC", CharacterSet8bit::unicode_map_WE8DEC);
        characterMap[3] = new CharacterSet8bit("WE8HP", CharacterSet8bit::unicode_map_WE8HP, true);
        characterMap[4] = new CharacterSet8bit("US8PC437", CharacterSet8bit::unicode_map_US8PC437);
        characterMap[10] = new CharacterSet8bit("WE8PC850", CharacterSet8bit::unicode_map_WE8PC850);
        characterMap[11] = new CharacterSet7bit("D7DEC", CharacterSet7bit::unicode_map_D7DEC);
        characterMap[13] = new CharacterSet7bit("S7DEC", CharacterSet7bit::unicode_map_S7DEC);
        characterMap[14] = new CharacterSet7bit("E7DEC", CharacterSet7bit::unicode_map_E7DEC);
        characterMap[15] = new CharacterSet7bit("SF7ASCII", CharacterSet7bit::unicode_map_SF7ASCII);
        characterMap[16] = new CharacterSet7bit("NDK7DEC", CharacterSet7bit::unicode_map_NDK7DEC);
        characterMap[17] = new CharacterSet7bit("I7DEC", CharacterSet7bit::unicode_map_I7DEC);
        characterMap[21] = new CharacterSet7bit("SF7DEC", CharacterSet7bit::unicode_map_SF7DEC);
        characterMap[25] = new CharacterSet8bit("IN8ISCII", CharacterSet8bit::unicode_map_IN8ISCII);
        characterMap[28] = new CharacterSet8bit("WE8PC858", CharacterSet8bit::unicode_map_WE8PC858);
        characterMap[31] = new CharacterSet8bit("WE8ISO8859P1", CharacterSet8bit::unicode_map_WE8ISO8859P1);
        characterMap[32] = new CharacterSet8bit("EE8ISO8859P2", CharacterSet8bit::unicode_map_EE8ISO8859P2);
        characterMap[33] = new CharacterSet8bit("SE8ISO8859P3", CharacterSet8bit::unicode_map_SE8ISO8859P3);
        characterMap[34] = new CharacterSet8bit("NEE8ISO8859P4", CharacterSet8bit::unicode_map_NEE8ISO8859P4);
        characterMap[35] = new CharacterSet8bit("CL8ISO8859P5", CharacterSet8bit::unicode_map_CL8ISO8859P5);
        characterMap[36] = new CharacterSet8bit("AR8ISO8859P6", CharacterSet8bit::unicode_map_AR8ISO8859P6);
        characterMap[37] = new CharacterSet8bit("EL8ISO8859P7", CharacterSet8bit::unicode_map_EL8ISO8859P7);
        characterMap[38] = new CharacterSet8bit("IW8ISO8859P8", CharacterSet8bit::unicode_map_IW8ISO8859P8);
        characterMap[39] = new CharacterSet8bit("WE8ISO8859P9", CharacterSet8bit::unicode_map_WE8ISO8859P9);
        characterMap[40] = new CharacterSet8bit("NE8ISO8859P10", CharacterSet8bit::unicode_map_NE8ISO8859P10);
        characterMap[41] = new CharacterSet8bit("TH8TISASCII", CharacterSet8bit::unicode_map_TH8TISASCII);
        characterMap[43] = new CharacterSet8bit("BN8BSCII", CharacterSet8bit::unicode_map_BN8BSCII);
        characterMap[44] = new CharacterSet8bit("VN8VN3", CharacterSet8bit::unicode_map_VN8VN3);
        characterMap[45] = new CharacterSet8bit("VN8MSWIN1258", CharacterSet8bit::unicode_map_VN8MSWIN1258);
        characterMap[46] = new CharacterSet8bit("WE8ISO8859P15", CharacterSet8bit::unicode_map_WE8ISO8859P15);
        characterMap[47] = new CharacterSet8bit("BLT8ISO8859P13", CharacterSet8bit::unicode_map_BLT8ISO8859P13);
        characterMap[48] = new CharacterSet8bit("CEL8ISO8859P14", CharacterSet8bit::unicode_map_CEL8ISO8859P14);
        characterMap[49] = new CharacterSet8bit("CL8ISOIR111", CharacterSet8bit::unicode_map_CL8ISOIR111);
        characterMap[50] = new CharacterSet8bit("WE8NEXTSTEP", CharacterSet8bit::unicode_map_WE8NEXTSTEP);
        characterMap[51] = new CharacterSet8bit("CL8KOI8U", CharacterSet8bit::unicode_map_CL8KOI8U);
        characterMap[52] = new CharacterSet8bit("AZ8ISO8859P9E", CharacterSet8bit::unicode_map_AZ8ISO8859P9E);
        characterMap[61] = new CharacterSet8bit("AR8ASMO708PLUS", CharacterSet8bit::unicode_map_AR8ASMO708PLUS);
        characterMap[81] = new CharacterSet8bit("EL8DEC", CharacterSet8bit::unicode_map_EL8DEC);
        characterMap[82] = new CharacterSet8bit("TR8DEC", CharacterSet8bit::unicode_map_TR8DEC);
        characterMap[110] = new CharacterSet8bit("EEC8EUROASCI", CharacterSet8bit::unicode_map_EEC8EUROASCI, true);
        characterMap[113] = new CharacterSet8bit("EEC8EUROPA3", CharacterSet8bit::unicode_map_EEC8EUROPA3, true);
        characterMap[114] = new CharacterSet8bit("LA8PASSPORT", CharacterSet8bit::unicode_map_LA8PASSPORT);
        characterMap[140] = new CharacterSet8bit("BG8PC437S", CharacterSet8bit::unicode_map_BG8PC437S);
        characterMap[150] = new CharacterSet8bit("EE8PC852", CharacterSet8bit::unicode_map_EE8PC852);
        characterMap[152] = new CharacterSet8bit("RU8PC866", CharacterSet8bit::unicode_map_RU8PC866);
        characterMap[153] = new CharacterSet8bit("RU8BESTA", CharacterSet8bit::unicode_map_RU8BESTA);
        characterMap[154] = new CharacterSet8bit("IW8PC1507", CharacterSet8bit::unicode_map_IW8PC1507);
        characterMap[155] = new CharacterSet8bit("RU8PC855", CharacterSet8bit::unicode_map_RU8PC855);
        characterMap[156] = new CharacterSet8bit("TR8PC857", CharacterSet8bit::unicode_map_TR8PC857);
        characterMap[159] = new CharacterSet8bit("CL8MACCYRILLICS", CharacterSet8bit::unicode_map_CL8MACCYRILLICS);
        characterMap[160] = new CharacterSet8bit("WE8PC860", CharacterSet8bit::unicode_map_WE8PC860);
        characterMap[161] = new CharacterSet8bit("IS8PC861", CharacterSet8bit::unicode_map_IS8PC861);
        characterMap[162] = new CharacterSet8bit("EE8MACCES", CharacterSet8bit::unicode_map_EE8MACCES);
        characterMap[163] = new CharacterSet8bit("EE8MACCROATIANS", CharacterSet8bit::unicode_map_EE8MACCROATIANS);
        characterMap[164] = new CharacterSet8bit("TR8MACTURKISHS", CharacterSet8bit::unicode_map_TR8MACTURKISHS);
        characterMap[165] = new CharacterSet8bit("IS8MACICELANDICS", CharacterSet8bit::unicode_map_IS8MACICELANDICS, true);
        characterMap[166] = new CharacterSet8bit("EL8MACGREEKS", CharacterSet8bit::unicode_map_EL8MACGREEKS);
        characterMap[167] = new CharacterSet8bit("IW8MACHEBREWS", CharacterSet8bit::unicode_map_IW8MACHEBREWS);
        characterMap[170] = new CharacterSet8bit("EE8MSWIN1250", CharacterSet8bit::unicode_map_EE8MSWIN1250);
        characterMap[171] = new CharacterSet8bit("CL8MSWIN1251", CharacterSet8bit::unicode_map_CL8MSWIN1251);
        characterMap[172] = new CharacterSet8bit("ET8MSWIN923", CharacterSet8bit::unicode_map_ET8MSWIN923);
        characterMap[173] = new CharacterSet8bit("BG8MSWIN", CharacterSet8bit::unicode_map_BG8MSWIN);
        characterMap[174] = new CharacterSet8bit("EL8MSWIN1253", CharacterSet8bit::unicode_map_EL8MSWIN1253);
        characterMap[175] = new CharacterSet8bit("IW8MSWIN1255", CharacterSet8bit::unicode_map_IW8MSWIN1255);
        characterMap[176] = new CharacterSet8bit("LT8MSWIN921", CharacterSet8bit::unicode_map_LT8MSWIN921);
        characterMap[177] = new CharacterSet8bit("TR8MSWIN1254", CharacterSet8bit::unicode_map_TR8MSWIN1254);
        characterMap[178] = new CharacterSet8bit("WE8MSWIN1252", CharacterSet8bit::unicode_map_WE8MSWIN1252);
        characterMap[179] = new CharacterSet8bit("BLT8MSWIN1257", CharacterSet8bit::unicode_map_BLT8MSWIN1257);
        characterMap[190] = new CharacterSet8bit("N8PC865", CharacterSet8bit::unicode_map_N8PC865);
        characterMap[191] = new CharacterSet8bit("BLT8CP921", CharacterSet8bit::unicode_map_BLT8CP921);
        characterMap[192] = new CharacterSet8bit("LV8PC1117", CharacterSet8bit::unicode_map_LV8PC1117);
        characterMap[193] = new CharacterSet8bit("LV8PC8LR", CharacterSet8bit::unicode_map_LV8PC8LR);
        characterMap[195] = new CharacterSet8bit("LV8RST104090", CharacterSet8bit::unicode_map_LV8RST104090);
        characterMap[196] = new CharacterSet8bit("CL8KOI8R", CharacterSet8bit::unicode_map_CL8KOI8R);
        characterMap[197] = new CharacterSet8bit("BLT8PC775", CharacterSet8bit::unicode_map_BLT8PC775);
        characterMap[202] = new CharacterSet7bit("E7SIEMENS9780X", CharacterSet7bit::unicode_map_E7SIEMENS9780X);
        characterMap[203] = new CharacterSet7bit("S7SIEMENS9780X", CharacterSet7bit::unicode_map_S7SIEMENS9780X);
        characterMap[204] = new CharacterSet7bit("DK7SIEMENS9780X", CharacterSet7bit::unicode_map_DK7SIEMENS9780X);
        characterMap[206] = new CharacterSet7bit("I7SIEMENS9780X", CharacterSet7bit::unicode_map_I7SIEMENS9780X);
        characterMap[205] = new CharacterSet7bit("N7SIEMENS9780X", CharacterSet7bit::unicode_map_N7SIEMENS9780X);
        characterMap[207] = new CharacterSet7bit("D7SIEMENS9780X", CharacterSet7bit::unicode_map_D7SIEMENS9780X);
        characterMap[241] = new CharacterSet8bit("WE8DG", CharacterSet8bit::unicode_map_WE8DG);
        characterMap[251] = new CharacterSet8bit("WE8NCR4970", CharacterSet8bit::unicode_map_WE8NCR4970);
        characterMap[261] = new CharacterSet8bit("WE8ROMAN8", CharacterSet8bit::unicode_map_WE8ROMAN8);
        characterMap[352] = new CharacterSet8bit("WE8MACROMAN8S", CharacterSet8bit::unicode_map_WE8MACROMAN8S);
        characterMap[354] = new CharacterSet8bit("TH8MACTHAIS", CharacterSet8bit::unicode_map_TH8MACTHAIS);
        characterMap[368] = new CharacterSet8bit("HU8CWI2", CharacterSet8bit::unicode_map_HU8CWI2);
        characterMap[380] = new CharacterSet8bit("EL8PC437S", CharacterSet8bit::unicode_map_EL8PC437S);
        characterMap[382] = new CharacterSet8bit("EL8PC737", CharacterSet8bit::unicode_map_EL8PC737);
        characterMap[383] = new CharacterSet8bit("LT8PC772", CharacterSet8bit::unicode_map_LT8PC772);
        characterMap[384] = new CharacterSet8bit("LT8PC774", CharacterSet8bit::unicode_map_LT8PC774);
        characterMap[385] = new CharacterSet8bit("EL8PC869", CharacterSet8bit::unicode_map_EL8PC869);
        characterMap[386] = new CharacterSet8bit("EL8PC851", CharacterSet8bit::unicode_map_EL8PC851);
        characterMap[390] = new CharacterSet8bit("CDN8PC863", CharacterSet8bit::unicode_map_CDN8PC863);
        characterMap[401] = new CharacterSet8bit("HU8ABMOD", CharacterSet8bit::unicode_map_HU8ABMOD);
        characterMap[500] = new CharacterSet8bit("AR8ASMO8X", CharacterSet8bit::unicode_map_AR8ASMO8X);
        characterMap[504] = new CharacterSet8bit("AR8NAFITHA711T", CharacterSet8bit::unicode_map_AR8NAFITHA711T);
        characterMap[505] = new CharacterSet8bit("AR8SAKHR707T", CharacterSet8bit::unicode_map_AR8SAKHR707T);
        characterMap[506] = new CharacterSet8bit("AR8MUSSAD768T", CharacterSet8bit::unicode_map_AR8MUSSAD768T);
        characterMap[507] = new CharacterSet8bit("AR8ADOS710T", CharacterSet8bit::unicode_map_AR8ADOS710T);
        characterMap[508] = new CharacterSet8bit("AR8ADOS720T", CharacterSet8bit::unicode_map_AR8ADOS720T);
        characterMap[509] = new CharacterSet8bit("AR8APTEC715T", CharacterSet8bit::unicode_map_AR8APTEC715T);
        characterMap[511] = new CharacterSet8bit("AR8NAFITHA721T", CharacterSet8bit::unicode_map_AR8NAFITHA721T);
        characterMap[514] = new CharacterSet8bit("AR8HPARABIC8T", CharacterSet8bit::unicode_map_AR8HPARABIC8T);
        characterMap[554] = new CharacterSet8bit("AR8NAFITHA711", CharacterSet8bit::unicode_map_AR8NAFITHA711);
        characterMap[555] = new CharacterSet8bit("AR8SAKHR707", CharacterSet8bit::unicode_map_AR8SAKHR707);
        characterMap[556] = new CharacterSet8bit("AR8MUSSAD768", CharacterSet8bit::unicode_map_AR8MUSSAD768);
        characterMap[557] = new CharacterSet8bit("AR8ADOS710", CharacterSet8bit::unicode_map_AR8ADOS710);
        characterMap[558] = new CharacterSet8bit("AR8ADOS720", CharacterSet8bit::unicode_map_AR8ADOS720);
        characterMap[559] = new CharacterSet8bit("AR8APTEC715", CharacterSet8bit::unicode_map_AR8APTEC715);
        characterMap[560] = new CharacterSet8bit("AR8MSWIN1256", CharacterSet8bit::unicode_map_AR8MSWIN1256);
        characterMap[561] = new CharacterSet8bit("AR8NAFITHA721", CharacterSet8bit::unicode_map_AR8NAFITHA721);
        characterMap[563] = new CharacterSet8bit("AR8SAKHR706", CharacterSet8bit::unicode_map_AR8SAKHR706);
        characterMap[566] = new CharacterSet8bit("AR8ARABICMACS", CharacterSet8bit::unicode_map_AR8ARABICMACS);
        characterMap[590] = new CharacterSet8bit("LA8ISO6937", CharacterSet8bit::unicode_map_LA8ISO6937);
        characterMap[829] = new CharacterSet16bit("JA16VMS", CharacterSet16bit::unicode_map_JA16VMS, JA16VMS_b1_min, JA16VMS_b1_max, JA16VMS_b2_min, JA16VMS_b2_max);
        characterMap[830] = new CharacterSetJA16EUC();
        characterMap[831] = new CharacterSetJA16EUC("JA16EUCYEN");
        characterMap[832] = new CharacterSetJA16SJIS();
        characterMap[834] = new CharacterSetJA16SJIS("JA16SJISYEN");
        characterMap[837] = new CharacterSetJA16EUCTILDE();
        characterMap[838] = new CharacterSetJA16SJISTILDE();
        characterMap[840] = new CharacterSet16bit("KO16KSC5601", CharacterSet16bit::unicode_map_KO16KSC5601_2b, KO16KSC5601_b1_min, KO16KSC5601_b1_max, KO16KSC5601_b2_min, KO16KSC5601_b2_max);
        characterMap[845] = new CharacterSetKO16KSCCS();
        characterMap[846] = new CharacterSet16bit("KO16MSWIN949", CharacterSet16bit::unicode_map_KO16MSWIN949_2b, KO16MSWIN949_b1_min, KO16MSWIN949_b1_max, KO16MSWIN949_b2_min, KO16MSWIN949_b2_max);
        characterMap[850] = new CharacterSet16bit("ZHS16CGB231280", CharacterSet16bit::unicode_map_ZHS16CGB231280_2b, ZHS16CGB231280_b1_min, ZHS16CGB231280_b1_max, ZHS16CGB231280_b2_min, ZHS16CGB231280_b2_max);
        characterMap[852] = new CharacterSetZHS16GBK();
        characterMap[854] = new CharacterSetZHS32GB18030();
        characterMap[860] = new CharacterSetZHT32EUC();
        characterMap[863] = new CharacterSetZHT32TRIS();
        characterMap[865] = new CharacterSet16bit("ZHT16BIG5", CharacterSet16bit::unicode_map_ZHT16BIG5_2b, ZHT16BIG5_b1_min, ZHT16BIG5_b1_max, ZHT16BIG5_b2_min, ZHT16BIG5_b2_max);
        characterMap[866] = new CharacterSet16bit("ZHT16CCDC", CharacterSet16bit::unicode_map_ZHT16CCDC_2b, ZHT16CCDC_b1_min, ZHT16CCDC_b1_max, ZHT16CCDC_b2_min, ZHT16CCDC_b2_max);
        characterMap[867] = new CharacterSet16bit("ZHT16MSWIN950", CharacterSet16bit::unicode_map_ZHT16MSWIN950_2b, ZHT16MSWIN950_b1_min, ZHT16MSWIN950_b1_max, ZHT16MSWIN950_b2_min, ZHT16MSWIN950_b2_max);
        characterMap[868] = new CharacterSet16bit("ZHT16HKSCS", CharacterSet16bit::unicode_map_ZHT16HKSCS_2b, ZHT16HKSCS_b1_min, ZHT16HKSCS_b1_max, ZHT16HKSCS_b2_min, ZHT16HKSCS_b2_max);
        characterMap[871] = new CharacterSetUTF8();
        characterMap[873] = new CharacterSetAL32UTF8();
        characterMap[992] = new CharacterSetZHT16HKSCS31();
        characterMap[1002] = new CharacterSet8bit("TIMESTEN8", CharacterSet8bit::unicode_map_TIMESTEN8);
        characterMap[2000] = new CharacterSetAL16UTF16();

        timeZoneMap[0x80a8] = "Africa/Abidjan";
        timeZoneMap[0x80c8] = "Africa/Accra";
        timeZoneMap[0x80bc] = "Africa/Addis_Ababa";
        timeZoneMap[0x8078] = "Africa/Algiers";
        timeZoneMap[0x80b8] = "Africa/Asmara";
        timeZoneMap[0x88b8] = "Africa/Asmera";
        timeZoneMap[0x80e8] = "Africa/Bamako";
        timeZoneMap[0x8094] = "Africa/Bangui";
        timeZoneMap[0x80c4] = "Africa/Banjul";
        timeZoneMap[0x80d0] = "Africa/Bissau";
        timeZoneMap[0x80e4] = "Africa/Blantyre";
        timeZoneMap[0x80a4] = "Africa/Brazzaville";
        timeZoneMap[0x808c] = "Africa/Bujumbura";
        timeZoneMap[0x80b0] = "Africa/Cairo";
        timeZoneMap[0x80f4] = "Africa/Casablanca";
        timeZoneMap[0x8144] = "Africa/Ceuta";
        timeZoneMap[0x80cc] = "Africa/Conakry";
        timeZoneMap[0x8114] = "Africa/Dakar";
        timeZoneMap[0x812c] = "Africa/Dar_es_Salaam";
        timeZoneMap[0x80ac] = "Africa/Djibouti";
        timeZoneMap[0x8090] = "Africa/Douala";
        timeZoneMap[0x80f8] = "Africa/El_Aaiun";
        timeZoneMap[0x8118] = "Africa/Freetown";
        timeZoneMap[0x8084] = "Africa/Gaborone";
        timeZoneMap[0x8140] = "Africa/Harare";
        timeZoneMap[0x8120] = "Africa/Johannesburg";
        timeZoneMap[0x8504] = "Africa/Juba";
        timeZoneMap[0x8138] = "Africa/Kampala";
        timeZoneMap[0x8124] = "Africa/Khartoum";
        timeZoneMap[0x810c] = "Africa/Kigali";
        timeZoneMap[0x809c] = "Africa/Kinshasa";
        timeZoneMap[0x8108] = "Africa/Lagos";
        timeZoneMap[0x80c0] = "Africa/Libreville";
        timeZoneMap[0x8130] = "Africa/Lome";
        timeZoneMap[0x807c] = "Africa/Luanda";
        timeZoneMap[0x80a0] = "Africa/Lubumbashi";
        timeZoneMap[0x813c] = "Africa/Lusaka";
        timeZoneMap[0x80b4] = "Africa/Malabo";
        timeZoneMap[0x80fc] = "Africa/Maputo";
        timeZoneMap[0x80d8] = "Africa/Maseru";
        timeZoneMap[0x8128] = "Africa/Mbabane";
        timeZoneMap[0x811c] = "Africa/Mogadishu";
        timeZoneMap[0x80dc] = "Africa/Monrovia";
        timeZoneMap[0x80d4] = "Africa/Nairobi";
        timeZoneMap[0x8098] = "Africa/Ndjamena";
        timeZoneMap[0x8104] = "Africa/Niamey";
        timeZoneMap[0x80f0] = "Africa/Nouakchott";
        timeZoneMap[0x8088] = "Africa/Ouagadougou";
        timeZoneMap[0x8080] = "Africa/Porto-Novo";
        timeZoneMap[0x8110] = "Africa/Sao_Tome";
        timeZoneMap[0x88e8] = "Africa/Timbuktu";
        timeZoneMap[0x80e0] = "Africa/Tripoli";
        timeZoneMap[0x8134] = "Africa/Tunis";
        timeZoneMap[0x8100] = "Africa/Windhoek";
        timeZoneMap[0x81b0] = "America/Adak";
        timeZoneMap[0x81a8] = "America/Anchorage";
        timeZoneMap[0x8248] = "America/Anguilla";
        timeZoneMap[0x824c] = "America/Antigua";
        timeZoneMap[0x82e8] = "America/Araguaina";
        timeZoneMap[0x8abc] = "America/Argentina/Buenos_Aires";
        timeZoneMap[0x8acc] = "America/Argentina/Catamarca";
        timeZoneMap[0x92cc] = "America/Argentina/ComodRivadavia";
        timeZoneMap[0x8ac4] = "America/Argentina/Cordoba";
        timeZoneMap[0x8ac8] = "America/Argentina/Jujuy";
        timeZoneMap[0x818c] = "America/Argentina/La_Rioja";
        timeZoneMap[0x8ad0] = "America/Argentina/Mendoza";
        timeZoneMap[0x8188] = "America/Argentina/Rio_Gallegos";
        timeZoneMap[0x83b4] = "America/Argentina/Salta";
        timeZoneMap[0x8394] = "America/Argentina/San_Juan";
        timeZoneMap[0x8184] = "America/Argentina/San_Luis";
        timeZoneMap[0x8390] = "America/Argentina/Tucuman";
        timeZoneMap[0x82c0] = "America/Argentina/Ushuaia";
        timeZoneMap[0x82d4] = "America/Aruba";
        timeZoneMap[0x8320] = "America/Asuncion";
        timeZoneMap[0x8374] = "America/Atikokan";
        timeZoneMap[0x89b0] = "America/Atka";
        timeZoneMap[0x8168] = "America/Bahia";
        timeZoneMap[0x817c] = "America/Bahia_Banderas";
        timeZoneMap[0x8254] = "America/Barbados";
        timeZoneMap[0x82e0] = "America/Belem";
        timeZoneMap[0x8258] = "America/Belize";
        timeZoneMap[0x8380] = "America/Blanc-Sablon";
        timeZoneMap[0x82fc] = "America/Boa_Vista";
        timeZoneMap[0x830c] = "America/Bogota";
        timeZoneMap[0x81b8] = "America/Boise";
        timeZoneMap[0x82bc] = "America/Buenos_Aires";
        timeZoneMap[0x821c] = "America/Cambridge_Bay";
        timeZoneMap[0x8378] = "America/Campo_Grande";
        timeZoneMap[0x8230] = "America/Cancun";
        timeZoneMap[0x8334] = "America/Caracas";
        timeZoneMap[0x82cc] = "America/Catamarca";
        timeZoneMap[0x8318] = "America/Cayenne";
        timeZoneMap[0x825c] = "America/Cayman";
        timeZoneMap[0x8194] = "America/Chicago";
        timeZoneMap[0x8238] = "America/Chihuahua";
        timeZoneMap[0x8b74] = "America/Coral_Harbour";
        timeZoneMap[0x82c4] = "America/Cordoba";
        timeZoneMap[0x8260] = "America/Costa_Rica";
        timeZoneMap[0x8514] = "America/Creston";
        timeZoneMap[0x82f4] = "America/Cuiaba";
        timeZoneMap[0x8310] = "America/Curacao";
        timeZoneMap[0x837c] = "America/Danmarkshavn";
        timeZoneMap[0x822c] = "America/Dawson";
        timeZoneMap[0x820c] = "America/Dawson_Creek";
        timeZoneMap[0x8198] = "America/Denver";
        timeZoneMap[0x81d0] = "America/Detroit";
        timeZoneMap[0x8268] = "America/Dominica";
        timeZoneMap[0x8204] = "America/Edmonton";
        timeZoneMap[0x8384] = "America/Eirunepe";
        timeZoneMap[0x8270] = "America/El_Salvador";
        timeZoneMap[0x8a44] = "America/Ensenada";
        timeZoneMap[0x82e4] = "America/Fortaleza";
        timeZoneMap[0x855c] = "America/Fort_Nelson";
        timeZoneMap[0x89bc] = "America/Fort_Wayne";
        timeZoneMap[0x81e4] = "America/Glace_Bay";
        timeZoneMap[0x833c] = "America/Godthab";
        timeZoneMap[0x81dc] = "America/Goose_Bay";
        timeZoneMap[0x82b0] = "America/Grand_Turk";
        timeZoneMap[0x8274] = "America/Grenada";
        timeZoneMap[0x8278] = "America/Guadeloupe";
        timeZoneMap[0x827c] = "America/Guatemala";
        timeZoneMap[0x8314] = "America/Guayaquil";
        timeZoneMap[0x831c] = "America/Guyana";
        timeZoneMap[0x81e0] = "America/Halifax";
        timeZoneMap[0x8264] = "America/Havana";
        timeZoneMap[0x823c] = "America/Hermosillo";
        timeZoneMap[0x99bc] = "America/Indiana/Indianapolis";
        timeZoneMap[0x81c4] = "America/Indiana/Knox";
        timeZoneMap[0x81c0] = "America/Indiana/Marengo";
        timeZoneMap[0x8348] = "America/Indiana/Petersburg";
        timeZoneMap[0x81bc] = "America/Indianapolis";
        timeZoneMap[0x8178] = "America/Indiana/Tell_City";
        timeZoneMap[0x81c8] = "America/Indiana/Vevay";
        timeZoneMap[0x8344] = "America/Indiana/Vincennes";
        timeZoneMap[0x8368] = "America/Indiana/Winamac";
        timeZoneMap[0x8224] = "America/Inuvik";
        timeZoneMap[0x8214] = "America/Iqaluit";
        timeZoneMap[0x8288] = "America/Jamaica";
        timeZoneMap[0x82c8] = "America/Jujuy";
        timeZoneMap[0x81a0] = "America/Juneau";
        timeZoneMap[0x89cc] = "America/Kentucky/Louisville";
        timeZoneMap[0x816c] = "America/Kentucky/Monticello";
        timeZoneMap[0x89c4] = "America/Knox_IN";
        timeZoneMap[0x850c] = "America/Kralendijk";
        timeZoneMap[0x82d8] = "America/La_Paz";
        timeZoneMap[0x8324] = "America/Lima";
        timeZoneMap[0x819c] = "America/Los_Angeles";
        timeZoneMap[0x81cc] = "America/Louisville";
        timeZoneMap[0x8508] = "America/Lower_Princes";
        timeZoneMap[0x82ec] = "America/Maceio";
        timeZoneMap[0x8294] = "America/Managua";
        timeZoneMap[0x8300] = "America/Manaus";
        timeZoneMap[0x8a78] = "America/Marigot";
        timeZoneMap[0x828c] = "America/Martinique";
        timeZoneMap[0x815c] = "America/Matamoros";
        timeZoneMap[0x8240] = "America/Mazatlan";
        timeZoneMap[0x82d0] = "America/Mendoza";
        timeZoneMap[0x81d4] = "America/Menominee";
        timeZoneMap[0x8388] = "America/Merida";
        timeZoneMap[0x84fc] = "America/Metlakatla";
        timeZoneMap[0x8234] = "America/Mexico_City";
        timeZoneMap[0x82a8] = "America/Miquelon";
        timeZoneMap[0x8170] = "America/Moncton";
        timeZoneMap[0x838c] = "America/Monterrey";
        timeZoneMap[0x8330] = "America/Montevideo";
        timeZoneMap[0x81e8] = "America/Montreal";
        timeZoneMap[0x8290] = "America/Montserrat";
        timeZoneMap[0x8250] = "America/Nassau";
        timeZoneMap[0x8190] = "America/New_York";
        timeZoneMap[0x81f0] = "America/Nipigon";
        timeZoneMap[0x81ac] = "America/Nome";
        timeZoneMap[0x82dc] = "America/Noronha";
        timeZoneMap[0x8500] = "America/North_Dakota/Beulah";
        timeZoneMap[0x8160] = "America/North_Dakota/Center";
        timeZoneMap[0x8164] = "America/North_Dakota/New_Salem";
        timeZoneMap[0x8174] = "America/Ojinaga";
        timeZoneMap[0x8298] = "America/Panama";
        timeZoneMap[0x8210] = "America/Pangnirtung";
        timeZoneMap[0x8328] = "America/Paramaribo";
        timeZoneMap[0x81b4] = "America/Phoenix";
        timeZoneMap[0x8280] = "America/Port-au-Prince";
        timeZoneMap[0x8304] = "America/Porto_Acre";
        timeZoneMap[0x832c] = "America/Port_of_Spain";
        timeZoneMap[0x82f8] = "America/Porto_Velho";
        timeZoneMap[0x829c] = "America/Puerto_Rico";
        timeZoneMap[0x8628] = "America/Punta_Arenas";
        timeZoneMap[0x81f4] = "America/Rainy_River";
        timeZoneMap[0x8218] = "America/Rankin_Inlet";
        timeZoneMap[0x8158] = "America/Recife";
        timeZoneMap[0x81fc] = "America/Regina";
        timeZoneMap[0x836c] = "America/Resolute";
        timeZoneMap[0x9304] = "America/Rio_Branco";
        timeZoneMap[0x92c4] = "America/Rosario";
        timeZoneMap[0x8180] = "America/Santa_Isabel";
        timeZoneMap[0x814c] = "America/Santarem";
        timeZoneMap[0x8308] = "America/Santiago";
        timeZoneMap[0x826c] = "America/Santo_Domingo";
        timeZoneMap[0x82f0] = "America/Sao_Paulo";
        timeZoneMap[0x8338] = "America/Scoresbysund";
        timeZoneMap[0x9998] = "America/Shiprock";
        timeZoneMap[0x84f8] = "America/Sitka";
        timeZoneMap[0x9278] = "America/St_Barthelemy";
        timeZoneMap[0x81d8] = "America/St_Johns";
        timeZoneMap[0x82a0] = "America/St_Kitts";
        timeZoneMap[0x82a4] = "America/St_Lucia";
        timeZoneMap[0x82b8] = "America/St_Thomas";
        timeZoneMap[0x82ac] = "America/St_Vincent";
        timeZoneMap[0x8200] = "America/Swift_Current";
        timeZoneMap[0x8284] = "America/Tegucigalpa";
        timeZoneMap[0x8340] = "America/Thule";
        timeZoneMap[0x81ec] = "America/Thunder_Bay";
        timeZoneMap[0x8244] = "America/Tijuana";
        timeZoneMap[0x8370] = "America/Toronto";
        timeZoneMap[0x82b4] = "America/Tortola";
        timeZoneMap[0x8208] = "America/Vancouver";
        timeZoneMap[0x8ab8] = "America/Virgin";
        timeZoneMap[0x8228] = "America/Whitehorse";
        timeZoneMap[0x81f8] = "America/Winnipeg";
        timeZoneMap[0x81a4] = "America/Yakutat";
        timeZoneMap[0x8220] = "America/Yellowknife";
        timeZoneMap[0x8398] = "Antarctica/Casey";
        timeZoneMap[0x839c] = "Antarctica/Davis";
        timeZoneMap[0x83a4] = "Antarctica/DumontDUrville";
        timeZoneMap[0x8154] = "Antarctica/Macquarie";
        timeZoneMap[0x83a0] = "Antarctica/Mawson";
        timeZoneMap[0x83b0] = "Antarctica/McMurdo";
        timeZoneMap[0x83ac] = "Antarctica/Palmer";
        timeZoneMap[0x8148] = "Antarctica/Rothera";
        timeZoneMap[0x8bb0] = "Antarctica/South_Pole";
        timeZoneMap[0x83a8] = "Antarctica/Syowa";
        timeZoneMap[0x8524] = "Antarctica/Troll";
        timeZoneMap[0x80ec] = "Antarctica/Vostok";
        timeZoneMap[0x8e34] = "Arctic/Longyearbyen";
        timeZoneMap[0x84b8] = "Asia/Aden";
        timeZoneMap[0x8434] = "Asia/Almaty";
        timeZoneMap[0x8430] = "Asia/Amman";
        timeZoneMap[0x84e0] = "Asia/Anadyr";
        timeZoneMap[0x843c] = "Asia/Aqtau";
        timeZoneMap[0x8438] = "Asia/Aqtobe";
        timeZoneMap[0x84a4] = "Asia/Ashgabat";
        timeZoneMap[0x8ca4] = "Asia/Ashkhabad";
        timeZoneMap[0x85ac] = "Asia/Atyrau";
        timeZoneMap[0x8424] = "Asia/Baghdad";
        timeZoneMap[0x83cc] = "Asia/Bahrain";
        timeZoneMap[0x83c8] = "Asia/Baku";
        timeZoneMap[0x84a0] = "Asia/Bangkok";
        timeZoneMap[0x859c] = "Asia/Barnaul";
        timeZoneMap[0x8454] = "Asia/Beirut";
        timeZoneMap[0x8440] = "Asia/Bishkek";
        timeZoneMap[0x83d8] = "Asia/Brunei";
        timeZoneMap[0x8410] = "Asia/Calcutta";
        timeZoneMap[0x853c] = "Asia/Chita";
        timeZoneMap[0x84f0] = "Asia/Choibalsan";
        timeZoneMap[0x8bec] = "Asia/Chongqing";
        timeZoneMap[0x83ec] = "Asia/Chungking";
        timeZoneMap[0x8494] = "Asia/Colombo";
        timeZoneMap[0x83d0] = "Asia/Dacca";
        timeZoneMap[0x8498] = "Asia/Damascus";
        timeZoneMap[0x8bd0] = "Asia/Dhaka";
        timeZoneMap[0x840c] = "Asia/Dili";
        timeZoneMap[0x84a8] = "Asia/Dubai";
        timeZoneMap[0x849c] = "Asia/Dushanbe";
        timeZoneMap[0x85a8] = "Asia/Famagusta";
        timeZoneMap[0x8474] = "Asia/Gaza";
        timeZoneMap[0x83e4] = "Asia/Harbin";
        timeZoneMap[0x8510] = "Asia/Hebron";
        timeZoneMap[0x8cb4] = "Asia/Ho_Chi_Minh";
        timeZoneMap[0x83f8] = "Asia/Hong_Kong";
        timeZoneMap[0x8460] = "Asia/Hovd";
        timeZoneMap[0x84cc] = "Asia/Irkutsk";
        timeZoneMap[0x965c] = "Asia/Istanbul";
        timeZoneMap[0x8414] = "Asia/Jakarta";
        timeZoneMap[0x841c] = "Asia/Jayapura";
        timeZoneMap[0x8428] = "Asia/Jerusalem";
        timeZoneMap[0x83c0] = "Asia/Kabul";
        timeZoneMap[0x84dc] = "Asia/Kamchatka";
        timeZoneMap[0x8470] = "Asia/Karachi";
        timeZoneMap[0x83f4] = "Asia/Kashgar";
        timeZoneMap[0x8c74] = "Asia/Kathmandu";
        timeZoneMap[0x8468] = "Asia/Katmandu";
        timeZoneMap[0x8518] = "Asia/Khandyga";
        timeZoneMap[0x8c10] = "Asia/Kolkata";
        timeZoneMap[0x84c8] = "Asia/Krasnoyarsk";
        timeZoneMap[0x8458] = "Asia/Kuala_Lumpur";
        timeZoneMap[0x845c] = "Asia/Kuching";
        timeZoneMap[0x844c] = "Asia/Kuwait";
        timeZoneMap[0x8400] = "Asia/Macao";
        timeZoneMap[0x8c00] = "Asia/Macau";
        timeZoneMap[0x84d8] = "Asia/Magadan";
        timeZoneMap[0x8c18] = "Asia/Makassar";
        timeZoneMap[0x8478] = "Asia/Manila";
        timeZoneMap[0x846c] = "Asia/Muscat";
        timeZoneMap[0x8404] = "Asia/Nicosia";
        timeZoneMap[0x8150] = "Asia/Novokuznetsk";
        timeZoneMap[0x84c4] = "Asia/Novosibirsk";
        timeZoneMap[0x84c0] = "Asia/Omsk";
        timeZoneMap[0x84ec] = "Asia/Oral";
        timeZoneMap[0x83e0] = "Asia/Phnom_Penh";
        timeZoneMap[0x84e4] = "Asia/Pontianak";
        timeZoneMap[0x8448] = "Asia/Pyongyang";
        timeZoneMap[0x847c] = "Asia/Qatar";
        timeZoneMap[0x84e8] = "Asia/Qyzylorda";
        timeZoneMap[0x83dc] = "Asia/Rangoon";
        timeZoneMap[0x8480] = "Asia/Riyadh";
        timeZoneMap[0x84b4] = "Asia/Saigon";
        timeZoneMap[0x84f4] = "Asia/Sakhalin";
        timeZoneMap[0x84ac] = "Asia/Samarkand";
        timeZoneMap[0x8444] = "Asia/Seoul";
        timeZoneMap[0x83e8] = "Asia/Shanghai";
        timeZoneMap[0x8490] = "Asia/Singapore";
        timeZoneMap[0x8554] = "Asia/Srednekolymsk";
        timeZoneMap[0x83fc] = "Asia/Taipei";
        timeZoneMap[0x84b0] = "Asia/Tashkent";
        timeZoneMap[0x8408] = "Asia/Tbilisi";
        timeZoneMap[0x8420] = "Asia/Tehran";
        timeZoneMap[0x8c28] = "Asia/Tel_Aviv";
        timeZoneMap[0x8bd4] = "Asia/Thimbu";
        timeZoneMap[0x83d4] = "Asia/Thimphu";
        timeZoneMap[0x842c] = "Asia/Tokyo";
        timeZoneMap[0x85a0] = "Asia/Tomsk";
        timeZoneMap[0x8418] = "Asia/Ujung_Pandang";
        timeZoneMap[0x8464] = "Asia/Ulaanbaatar";
        timeZoneMap[0x8c64] = "Asia/Ulan_Bator";
        timeZoneMap[0x83f0] = "Asia/Urumqi";
        timeZoneMap[0x851c] = "Asia/Ust-Nera";
        timeZoneMap[0x8450] = "Asia/Vientiane";
        timeZoneMap[0x84d4] = "Asia/Vladivostok";
        timeZoneMap[0x84d0] = "Asia/Yakutsk";
        timeZoneMap[0x85a4] = "Asia/Yangon";
        timeZoneMap[0x84bc] = "Asia/Yekaterinburg";
        timeZoneMap[0x83c4] = "Asia/Yerevan";
        timeZoneMap[0x8540] = "Atlantic/Azores";
        timeZoneMap[0x8528] = "Atlantic/Bermuda";
        timeZoneMap[0x8548] = "Atlantic/Canary";
        timeZoneMap[0x854c] = "Atlantic/Cape_Verde";
        timeZoneMap[0x8d34] = "Atlantic/Faeroe";
        timeZoneMap[0x8534] = "Atlantic/Faroe";
        timeZoneMap[0x9634] = "Atlantic/Jan_Mayen";
        timeZoneMap[0x8544] = "Atlantic/Madeira";
        timeZoneMap[0x8538] = "Atlantic/Reykjavik";
        timeZoneMap[0x8530] = "Atlantic/South_Georgia";
        timeZoneMap[0x852c] = "Atlantic/Stanley";
        timeZoneMap[0x8550] = "Atlantic/St_Helena";
        timeZoneMap[0x8d80] = "Australia/ACT";
        timeZoneMap[0x8574] = "Australia/Adelaide";
        timeZoneMap[0x856c] = "Australia/Brisbane";
        timeZoneMap[0x8584] = "Australia/Broken_Hill";
        timeZoneMap[0x9580] = "Australia/Canberra";
        timeZoneMap[0x858c] = "Australia/Currie";
        timeZoneMap[0x8564] = "Australia/Darwin";
        timeZoneMap[0x8590] = "Australia/Eucla";
        timeZoneMap[0x8578] = "Australia/Hobart";
        timeZoneMap[0x8d88] = "Australia/LHI";
        timeZoneMap[0x8570] = "Australia/Lindeman";
        timeZoneMap[0x8588] = "Australia/Lord_Howe";
        timeZoneMap[0x857c] = "Australia/Melbourne";
        timeZoneMap[0x8d64] = "Australia/North";
        timeZoneMap[0x9d80] = "Australia/NSW";
        timeZoneMap[0x8568] = "Australia/Perth";
        timeZoneMap[0x8d6c] = "Australia/Queensland";
        timeZoneMap[0x8d74] = "Australia/South";
        timeZoneMap[0x8580] = "Australia/Sydney";
        timeZoneMap[0x8d78] = "Australia/Tasmania";
        timeZoneMap[0x8d7c] = "Australia/Victoria";
        timeZoneMap[0x8d68] = "Australia/West";
        timeZoneMap[0x8d84] = "Australia/Yancowinna";
        timeZoneMap[0x8b04] = "Brazil/Acre";
        timeZoneMap[0x8adc] = "Brazil/DeNoronha";
        timeZoneMap[0x8af0] = "Brazil/East";
        timeZoneMap[0x8b00] = "Brazil/West";
        timeZoneMap[0x89e0] = "Canada/Atlantic";
        timeZoneMap[0x89f8] = "Canada/Central";
        timeZoneMap[0x89e8] = "Canada/Eastern";
        timeZoneMap[0x89fc] = "Canada/East-Saskatchewan";
        timeZoneMap[0x8a04] = "Canada/Mountain";
        timeZoneMap[0x89d8] = "Canada/Newfoundland";
        timeZoneMap[0x8a08] = "Canada/Pacific";
        timeZoneMap[0x91fc] = "Canada/Saskatchewan";
        timeZoneMap[0x8a28] = "Canada/Yukon";
        timeZoneMap[0x85b8] = "CET";
        timeZoneMap[0x8b08] = "Chile/Continental";
        timeZoneMap[0x8f0c] = "Chile/EasterIsland";
        timeZoneMap[0x9994] = "CST";
        timeZoneMap[0x835c] = "CST6CDT";
        timeZoneMap[0x8a64] = "Cuba";
        timeZoneMap[0x85c0] = "EET";
        timeZoneMap[0x88b0] = "Egypt";
        timeZoneMap[0x8dcc] = "Eire";
        timeZoneMap[0x834c] = "EST";
        timeZoneMap[0x8358] = "EST5EDT";
        timeZoneMap[0x9004] = "Etc/GMT+0";
        timeZoneMap[0xa004] = "Etc/GMT-0";
        timeZoneMap[0xb004] = "Etc/GMT0";
        timeZoneMap[0x8004] = "Etc/GMT";
        timeZoneMap[0x8018] = "Etc/GMT-10";
        timeZoneMap[0x8064] = "Etc/GMT+10";
        timeZoneMap[0x803c] = "Etc/GMT-1";
        timeZoneMap[0x8040] = "Etc/GMT+1";
        timeZoneMap[0x8014] = "Etc/GMT-11";
        timeZoneMap[0x8068] = "Etc/GMT+11";
        timeZoneMap[0x8010] = "Etc/GMT-12";
        timeZoneMap[0x806c] = "Etc/GMT+12";
        timeZoneMap[0x800c] = "Etc/GMT-13";
        timeZoneMap[0x8008] = "Etc/GMT-14";
        timeZoneMap[0x8038] = "Etc/GMT-2";
        timeZoneMap[0x8044] = "Etc/GMT+2";
        timeZoneMap[0x8034] = "Etc/GMT-3";
        timeZoneMap[0x8048] = "Etc/GMT+3";
        timeZoneMap[0x8030] = "Etc/GMT-4";
        timeZoneMap[0x804c] = "Etc/GMT+4";
        timeZoneMap[0x802c] = "Etc/GMT-5";
        timeZoneMap[0x8050] = "Etc/GMT+5";
        timeZoneMap[0x8028] = "Etc/GMT-6";
        timeZoneMap[0x8054] = "Etc/GMT+6";
        timeZoneMap[0x8024] = "Etc/GMT-7";
        timeZoneMap[0x8058] = "Etc/GMT+7";
        timeZoneMap[0x8020] = "Etc/GMT-8";
        timeZoneMap[0x805c] = "Etc/GMT+8";
        timeZoneMap[0x801c] = "Etc/GMT-9";
        timeZoneMap[0x8060] = "Etc/GMT+9";
        timeZoneMap[0xc004] = "Etc/Greenwich";
        timeZoneMap[0x8074] = "Etc/UCT";
        timeZoneMap[0x8870] = "Etc/Universal";
        timeZoneMap[0x8070] = "Etc/UTC";
        timeZoneMap[0x9870] = "Etc/Zulu";
        timeZoneMap[0x8630] = "Europe/Amsterdam";
        timeZoneMap[0x85d4] = "Europe/Andorra";
        timeZoneMap[0x8560] = "Europe/Astrakhan";
        timeZoneMap[0x8604] = "Europe/Athens";
        timeZoneMap[0x85c8] = "Europe/Belfast";
        timeZoneMap[0x8670] = "Europe/Belgrade";
        timeZoneMap[0x85fc] = "Europe/Berlin";
        timeZoneMap[0x8de8] = "Europe/Bratislava";
        timeZoneMap[0x85e0] = "Europe/Brussels";
        timeZoneMap[0x8640] = "Europe/Bucharest";
        timeZoneMap[0x8608] = "Europe/Budapest";
        timeZoneMap[0x8520] = "Europe/Busingen";
        timeZoneMap[0x8624] = "Europe/Chisinau";
        timeZoneMap[0x85ec] = "Europe/Copenhagen";
        timeZoneMap[0x85cc] = "Europe/Dublin";
        timeZoneMap[0x8600] = "Europe/Gibraltar";
        timeZoneMap[0xa5c4] = "Europe/Guernsey";
        timeZoneMap[0x85f4] = "Europe/Helsinki";
        timeZoneMap[0xadc4] = "Europe/Isle_of_Man";
        timeZoneMap[0x865c] = "Europe/Istanbul";
        timeZoneMap[0x9dc4] = "Europe/Jersey";
        timeZoneMap[0x8644] = "Europe/Kaliningrad";
        timeZoneMap[0x8660] = "Europe/Kiev";
        timeZoneMap[0x8594] = "Europe/Kirov";
        timeZoneMap[0x863c] = "Europe/Lisbon";
        timeZoneMap[0x8e70] = "Europe/Ljubljana";
        timeZoneMap[0x85c4] = "Europe/London";
        timeZoneMap[0x861c] = "Europe/Luxembourg";
        timeZoneMap[0x8650] = "Europe/Madrid";
        timeZoneMap[0x8620] = "Europe/Malta";
        timeZoneMap[0x8df4] = "Europe/Mariehamn";
        timeZoneMap[0x85dc] = "Europe/Minsk";
        timeZoneMap[0x862c] = "Europe/Monaco";
        timeZoneMap[0x8648] = "Europe/Moscow";
        timeZoneMap[0x8c04] = "Europe/Nicosia";
        timeZoneMap[0x8634] = "Europe/Oslo";
        timeZoneMap[0x85f8] = "Europe/Paris";
        timeZoneMap[0xae70] = "Europe/Podgorica";
        timeZoneMap[0x85e8] = "Europe/Prague";
        timeZoneMap[0x8610] = "Europe/Riga";
        timeZoneMap[0x860c] = "Europe/Rome";
        timeZoneMap[0x864c] = "Europe/Samara";
        timeZoneMap[0x960c] = "Europe/San_Marino";
        timeZoneMap[0x9670] = "Europe/Sarajevo";
        timeZoneMap[0x85b0] = "Europe/Saratov";
        timeZoneMap[0x866c] = "Europe/Simferopol";
        timeZoneMap[0x9e70] = "Europe/Skopje";
        timeZoneMap[0x85e4] = "Europe/Sofia";
        timeZoneMap[0x8654] = "Europe/Stockholm";
        timeZoneMap[0x85f0] = "Europe/Tallinn";
        timeZoneMap[0x85d0] = "Europe/Tirane";
        timeZoneMap[0x8e24] = "Europe/Tiraspol";
        timeZoneMap[0x8598] = "Europe/Ulyanovsk";
        timeZoneMap[0x8664] = "Europe/Uzhgorod";
        timeZoneMap[0x8614] = "Europe/Vaduz";
        timeZoneMap[0x8e0c] = "Europe/Vatican";
        timeZoneMap[0x85d8] = "Europe/Vienna";
        timeZoneMap[0x8618] = "Europe/Vilnius";
        timeZoneMap[0x8674] = "Europe/Volgograd";
        timeZoneMap[0x8638] = "Europe/Warsaw";
        timeZoneMap[0xa670] = "Europe/Zagreb";
        timeZoneMap[0x8668] = "Europe/Zaporozhye";
        timeZoneMap[0x8658] = "Europe/Zurich";
        timeZoneMap[0x8dc4] = "GB";
        timeZoneMap[0x95c4] = "GB-Eire";
        timeZoneMap[0x9804] = "GMT+0";
        timeZoneMap[0xa804] = "GMT-0";
        timeZoneMap[0xb804] = "GMT0";
        timeZoneMap[0x8804] = "GMT";
        timeZoneMap[0xc804] = "Greenwich";
        timeZoneMap[0x8bf8] = "Hongkong";
        timeZoneMap[0x8354] = "HST";
        timeZoneMap[0x8d38] = "Iceland";
        timeZoneMap[0x86d8] = "Indian/Antananarivo";
        timeZoneMap[0x86d0] = "Indian/Chagos";
        timeZoneMap[0x86dc] = "Indian/Christmas";
        timeZoneMap[0x86e0] = "Indian/Cocos";
        timeZoneMap[0x86e4] = "Indian/Comoro";
        timeZoneMap[0x86cc] = "Indian/Kerguelen";
        timeZoneMap[0x86e8] = "Indian/Mahe";
        timeZoneMap[0x86d4] = "Indian/Maldives";
        timeZoneMap[0x86ec] = "Indian/Mauritius";
        timeZoneMap[0x86f0] = "Indian/Mayotte";
        timeZoneMap[0x86f4] = "Indian/Reunion";
        timeZoneMap[0x8c20] = "Iran";
        timeZoneMap[0x9428] = "Israel";
        timeZoneMap[0x8a88] = "Jamaica";
        timeZoneMap[0x8c2c] = "Japan";
        timeZoneMap[0x8f40] = "Kwajalein";
        timeZoneMap[0x88e0] = "Libya";
        timeZoneMap[0x85bc] = "MET";
        timeZoneMap[0x9244] = "Mexico/BajaNorte";
        timeZoneMap[0x8a40] = "Mexico/BajaSur";
        timeZoneMap[0x8a34] = "Mexico/General";
        timeZoneMap[0x8350] = "MST";
        timeZoneMap[0x8360] = "MST7MDT";
        timeZoneMap[0x8998] = "Navajo";
        timeZoneMap[0x8f5c] = "NZ";
        timeZoneMap[0x8f60] = "NZ-CHAT";
        timeZoneMap[0x877c] = "Pacific/Apia";
        timeZoneMap[0x875c] = "Pacific/Auckland";
        timeZoneMap[0x8558] = "Pacific/Bougainville";
        timeZoneMap[0x8760] = "Pacific/Chatham";
        timeZoneMap[0x83b8] = "Pacific/Chuuk";
        timeZoneMap[0x870c] = "Pacific/Easter";
        timeZoneMap[0x87a0] = "Pacific/Efate";
        timeZoneMap[0x8730] = "Pacific/Enderbury";
        timeZoneMap[0x8788] = "Pacific/Fakaofo";
        timeZoneMap[0x8718] = "Pacific/Fiji";
        timeZoneMap[0x8790] = "Pacific/Funafuti";
        timeZoneMap[0x8710] = "Pacific/Galapagos";
        timeZoneMap[0x871c] = "Pacific/Gambier";
        timeZoneMap[0x8784] = "Pacific/Guadalcanal";
        timeZoneMap[0x8728] = "Pacific/Guam";
        timeZoneMap[0x8708] = "Pacific/Honolulu";
        timeZoneMap[0x8794] = "Pacific/Johnston";
        timeZoneMap[0x8734] = "Pacific/Kiritimati";
        timeZoneMap[0x8750] = "Pacific/Kosrae";
        timeZoneMap[0x8740] = "Pacific/Kwajalein";
        timeZoneMap[0x873c] = "Pacific/Majuro";
        timeZoneMap[0x8720] = "Pacific/Marquesas";
        timeZoneMap[0x8798] = "Pacific/Midway";
        timeZoneMap[0x8754] = "Pacific/Nauru";
        timeZoneMap[0x8764] = "Pacific/Niue";
        timeZoneMap[0x8768] = "Pacific/Norfolk";
        timeZoneMap[0x8758] = "Pacific/Noumea";
        timeZoneMap[0x8778] = "Pacific/Pago_Pago";
        timeZoneMap[0x876c] = "Pacific/Palau";
        timeZoneMap[0x8774] = "Pacific/Pitcairn";
        timeZoneMap[0x83bc] = "Pacific/Pohnpei";
        timeZoneMap[0x874c] = "Pacific/Ponape";
        timeZoneMap[0x8770] = "Pacific/Port_Moresby";
        timeZoneMap[0x8714] = "Pacific/Rarotonga";
        timeZoneMap[0x8738] = "Pacific/Saipan";
        timeZoneMap[0x9778] = "Pacific/Samoa";
        timeZoneMap[0x8724] = "Pacific/Tahiti";
        timeZoneMap[0x872c] = "Pacific/Tarawa";
        timeZoneMap[0x878c] = "Pacific/Tongatapu";
        timeZoneMap[0x8748] = "Pacific/Truk";
        timeZoneMap[0x879c] = "Pacific/Wake";
        timeZoneMap[0x87a4] = "Pacific/Wallis";
        timeZoneMap[0x8f48] = "Pacific/Yap";
        timeZoneMap[0x8e38] = "Poland";
        timeZoneMap[0x8e3c] = "Portugal";
        timeZoneMap[0x8be8] = "PRC";
        timeZoneMap[0xa19c] = "PST";
        timeZoneMap[0x8364] = "PST8PDT";
        timeZoneMap[0x8bfc] = "ROC";
        timeZoneMap[0x8c44] = "ROK";
        timeZoneMap[0x8c90] = "Singapore";
        timeZoneMap[0x8e5c] = "Turkey";
        timeZoneMap[0x8874] = "UCT";
        timeZoneMap[0x9070] = "Universal";
        timeZoneMap[0x89a8] = "US/Alaska";
        timeZoneMap[0x91b0] = "US/Aleutian";
        timeZoneMap[0x89b4] = "US/Arizona";
        timeZoneMap[0x8994] = "US/Central";
        timeZoneMap[0x8990] = "US/Eastern";
        timeZoneMap[0x91bc] = "US/East-Indiana";
        timeZoneMap[0x8f08] = "US/Hawaii";
        timeZoneMap[0x91c4] = "US/Indiana-Starke";
        timeZoneMap[0x89d0] = "US/Michigan";
        timeZoneMap[0x9198] = "US/Mountain";
        timeZoneMap[0x899c] = "US/Pacific";
        timeZoneMap[0x999c] = "US/Pacific-New";
        timeZoneMap[0x8f78] = "US/Samoa";
        timeZoneMap[0xd004] = "UTC";
        timeZoneMap[0x85b4] = "WET";
        timeZoneMap[0x8e48] = "W-SU";
        timeZoneMap[0xa070] = "Zulu";
    }

    OutputBuffer::~OutputBuffer() {
        for (auto it : characterMap) {
            CharacterSet *cs = it.second;
            delete cs;
        }
        characterMap.clear();
        timeZoneMap.clear();
        objects.clear();

        while (firstBuffer != nullptr) {
            uint8_t* nextBuffer = *((uint8_t**)(firstBuffer + OUTPUT_BUFFER_NEXT));
            oracleAnalyser->freeMemoryChunk("BUFFER", firstBuffer, true);
            firstBuffer = nextBuffer;
            --buffersAllocated;
        }
    }

    void OutputBuffer::initialize(OracleAnalyser *oracleAnalyser) {
        this->oracleAnalyser = oracleAnalyser;

        buffersAllocated = 1;
        firstBuffer = oracleAnalyser->getMemoryChunk("BUFFER", false);
        *((uint8_t**)(firstBuffer + OUTPUT_BUFFER_NEXT)) = nullptr;
        *((uint64_t*)(firstBuffer + OUTPUT_BUFFER_END)) = OUTPUT_BUFFER_DATA;
        firstBufferPos = OUTPUT_BUFFER_DATA;
        lastBuffer = firstBuffer;
        lastBufferPos = OUTPUT_BUFFER_DATA;
    }

    void OutputBuffer::bufferAppend(uint8_t character) {
        lastBuffer[lastBufferPos] = character;
        ++messageLength;
        bufferShift(1);
    }

    void OutputBuffer::bufferShift(uint64_t bytes) {
        lastBufferPos += bytes;

        if (lastBufferPos >= MEMORY_CHUNK_SIZE) {
            uint8_t *nextBuffer = oracleAnalyser->getMemoryChunk("BUFFER", true);
            *((uint8_t**)(nextBuffer + OUTPUT_BUFFER_NEXT)) = nullptr;
            *((uint64_t*)(nextBuffer + OUTPUT_BUFFER_END)) = OUTPUT_BUFFER_DATA;
            {
                unique_lock<mutex> lck(mtx);
                *((uint8_t**)(lastBuffer + OUTPUT_BUFFER_NEXT)) = nextBuffer;
                *((uint64_t*)(lastBuffer + OUTPUT_BUFFER_END)) = MEMORY_CHUNK_SIZE;
                ++buffersAllocated;
                lastBuffer = nextBuffer;
                lastBufferPos = OUTPUT_BUFFER_DATA;
            }
        }
    }

    void OutputBuffer::beginMessage(void) {
        curBuffer = lastBuffer;
        curBufferPos = lastBufferPos;
        messageLength = 0;
        *((uint64_t*)(lastBuffer + lastBufferPos)) = 0;
        bufferShift(OUTPUT_BUFFER_LENGTH_SIZE);
    }

    void OutputBuffer::commitMessage(void) {
        if (messageLength == 0) {
            WARNING("JSON buffer - commit of empty transaction");
        }

        bufferShift((8 - (messageLength & 7)) & 7);
        {
            unique_lock<mutex> lck(mtx);
            *((uint64_t*)(curBuffer + curBufferPos)) = messageLength;
            if (curBuffer != lastBuffer)
                *((uint64_t*)(curBuffer + OUTPUT_BUFFER_END)) = MEMORY_CHUNK_SIZE;
            *((uint64_t*)(lastBuffer + OUTPUT_BUFFER_END)) = lastBufferPos;
            writersCond.notify_all();
        }
    }

    uint64_t OutputBuffer::currentMessageSize(void) {
        return messageLength + OUTPUT_BUFFER_LENGTH_SIZE;
    }

    void OutputBuffer::setWriter(Writer *writer) {
        this->writer = writer;
    }

    void OutputBuffer::setNlsCharset(string &nlsCharset, string &nlsNcharCharset) {
        INFO("loading character mapping for " << nlsCharset);

        for (auto elem: characterMap) {
            if (strcmp(nlsCharset.c_str(), elem.second->name) == 0) {
                defaultCharacterMapId = elem.first;
                break;
            }
        }

        if (defaultCharacterMapId == 0) {
            RUNTIME_FAIL("unsupported NLS_CHARACTERSET value");
        }

        INFO("loading character mapping for " << nlsNcharCharset);
        for (auto elem: characterMap) {
            if (strcmp(nlsNcharCharset.c_str(), elem.second->name) == 0) {
                defaultCharacterNcharMapId = elem.first;
                break;
            }
        }

        if (defaultCharacterNcharMapId == 0) {
            RUNTIME_FAIL("unsupported NLS_NCHAR_CHARACTERSET value");
        }
    }

    void OutputBuffer::append(string &str) {
        const char *charstr = str.c_str();
        uint64_t length = str.length();
        for (uint i = 0; i < length; ++i)
            bufferAppend(*charstr++);
    }

    void OutputBuffer::append(const char *str) {
        char character = *str++;
        while (character != 0) {
            bufferAppend(character);
            character = *str++;
        }
    }

    void OutputBuffer::append(char chr) {
        bufferAppend(chr);
    }

    void OutputBuffer::checkUpdate(OracleObject *object, typedba bdba, typeslot slot, typexid xid) {
        if (columnFormat <= COLUMN_FORMAT_INS_DEC) {
            for (uint64_t i = 0; i < object->maxSegCol; ++i) {
                if (object->columns[i] == nullptr)
                    continue;

                //remove unchanged column values - only for tables with defined primary key
                if (object->columns[i]->numPk == 0 && beforePos[i] > 0 && afterPos[i] > 0 && beforeLen[i] == afterLen[i]) {
                    if (beforeLen[i] == 0 || memcmp(beforeRecord[i]->data + beforePos[i], afterRecord[i]->data + afterPos[i], beforeLen[i]) == 0) {
                        beforePos[i] = 0;
                        afterPos[i] = 0;
                        beforeLen[i] = 0;
                        afterLen[i] = 0;
                    }
                }

                //remove columns additionally present, but not modified
                if (beforePos[i] > 0 && beforeLen[i] == 0 && afterPos[i] == 0) {
                    if (object->columns[i]->numPk == 0) {
                        beforePos[i] = 0;
                    } else {
                        afterPos[i] = beforePos[i];
                        afterLen[i] = beforeLen[i];
                        afterRecord[i] = beforeRecord[i];
                    }
                }
                if (afterPos[i] > 0 && afterLen[i] == 0 && beforePos[i] == 0) {
                    if (object->columns[i]->numPk == 0) {
                        afterPos[i] = 0;
                    } else {
                        beforePos[i] = afterPos[i];
                        beforeLen[i] = afterLen[i];
                        beforeRecord[i] = afterRecord[i];
                    }
                }
            }
        }
    }
}
