/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: sbr_qmf_c.h,v 1.1 2008/08/15 01:12:39 zzinho Exp $
**/

#ifndef __SBR_QMF_C_H__
#define __SBR_QMF_C_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _MSC_VER
#pragma warning(disable:4305)
#pragma warning(disable:4244)
#endif

ALIGN static const real_t qmf_c[640] = {
    FRAC_CONST(0), FRAC_CONST(-0.00055252865047),
    FRAC_CONST(-0.00056176925738), FRAC_CONST(-0.00049475180896),
    FRAC_CONST(-0.00048752279712), FRAC_CONST(-0.00048937912498),
    FRAC_CONST(-0.00050407143497), FRAC_CONST(-0.00052265642972),
    FRAC_CONST(-0.00054665656337), FRAC_CONST(-0.00056778025613),
    FRAC_CONST(-0.00058709304852), FRAC_CONST(-0.00061327473938),
    FRAC_CONST(-0.00063124935319), FRAC_CONST(-0.00065403333621),
    FRAC_CONST(-0.00067776907764), FRAC_CONST(-0.00069416146273),
    FRAC_CONST(-0.00071577364744), FRAC_CONST(-0.00072550431222),
    FRAC_CONST(-0.00074409418541), FRAC_CONST(-0.00074905980532),
    FRAC_CONST(-0.0007681371927), FRAC_CONST(-0.00077248485949),
    FRAC_CONST(-0.00078343322877), FRAC_CONST(-0.00077798694927),
    FRAC_CONST(-0.000780366471), FRAC_CONST(-0.00078014496257),
    FRAC_CONST(-0.0007757977331), FRAC_CONST(-0.00076307935757),
    FRAC_CONST(-0.00075300014201), FRAC_CONST(-0.00073193571525),
    FRAC_CONST(-0.00072153919876), FRAC_CONST(-0.00069179375372),
    FRAC_CONST(-0.00066504150893), FRAC_CONST(-0.00063415949025),
    FRAC_CONST(-0.0005946118933), FRAC_CONST(-0.00055645763906),
    FRAC_CONST(-0.00051455722108), FRAC_CONST(-0.00046063254803),
    FRAC_CONST(-0.00040951214522), FRAC_CONST(-0.00035011758756),
    FRAC_CONST(-0.00028969811748), FRAC_CONST(-0.0002098337344),
    FRAC_CONST(-0.00014463809349), FRAC_CONST(-6.173344072E-005),
    FRAC_CONST(1.349497418E-005), FRAC_CONST(0.00010943831274),
    FRAC_CONST(0.00020430170688), FRAC_CONST(0.00029495311041),
    FRAC_CONST(0.0004026540216), FRAC_CONST(0.00051073884952),
    FRAC_CONST(0.00062393761391), FRAC_CONST(0.00074580258865),
    FRAC_CONST(0.00086084433262), FRAC_CONST(0.00098859883015),
    FRAC_CONST(0.00112501551307), FRAC_CONST(0.00125778846475),
    FRAC_CONST(0.00139024948272), FRAC_CONST(0.00154432198471),
    FRAC_CONST(0.00168680832531), FRAC_CONST(0.00183482654224),
    FRAC_CONST(0.00198411407369), FRAC_CONST(0.00214615835557),
    FRAC_CONST(0.00230172547746), FRAC_CONST(0.00246256169126),
    FRAC_CONST(0.00262017586902), FRAC_CONST(0.00278704643465),
    FRAC_CONST(0.00294694477165), FRAC_CONST(0.00311254206525),
    FRAC_CONST(0.00327396134847), FRAC_CONST(0.00344188741828),
    FRAC_CONST(0.00360082681231), FRAC_CONST(0.00376039229104),
    FRAC_CONST(0.00392074323703), FRAC_CONST(0.00408197531935),
    FRAC_CONST(0.0042264269227), FRAC_CONST(0.00437307196781),
    FRAC_CONST(0.00452098527825), FRAC_CONST(0.00466064606118),
    FRAC_CONST(0.00479325608498), FRAC_CONST(0.00491376035745),
    FRAC_CONST(0.00503930226013), FRAC_CONST(0.00514073539032),
    FRAC_CONST(0.00524611661324), FRAC_CONST(0.00534716811982),
    FRAC_CONST(0.00541967759307), FRAC_CONST(0.00548760401507),
    FRAC_CONST(0.00554757145088), FRAC_CONST(0.00559380230045),
    FRAC_CONST(0.00562206432097), FRAC_CONST(0.00564551969164),
    FRAC_CONST(0.00563891995151), FRAC_CONST(0.00562661141932),
    FRAC_CONST(0.0055917128663), FRAC_CONST(0.005540436394),
    FRAC_CONST(0.0054753783077), FRAC_CONST(0.0053838975897),
    FRAC_CONST(0.00527157587272), FRAC_CONST(0.00513822754514),
    FRAC_CONST(0.00498396877629), FRAC_CONST(0.004810946906),
    FRAC_CONST(0.00460395301471), FRAC_CONST(0.00438018617447),
    FRAC_CONST(0.0041251642327), FRAC_CONST(0.00384564081246),
    FRAC_CONST(0.00354012465507), FRAC_CONST(0.00320918858098),
    FRAC_CONST(0.00284467578623), FRAC_CONST(0.00245085400321),
    FRAC_CONST(0.0020274176185), FRAC_CONST(0.00157846825768),
    FRAC_CONST(0.00109023290512), FRAC_CONST(0.0005832264248),
    FRAC_CONST(2.760451905E-005), FRAC_CONST(-0.00054642808664),
    FRAC_CONST(-0.00115681355227), FRAC_CONST(-0.00180394725893),
    FRAC_CONST(-0.00248267236449), FRAC_CONST(-0.003193377839),
    FRAC_CONST(-0.00394011240522), FRAC_CONST(-0.004722259624),
    FRAC_CONST(-0.00553372111088), FRAC_CONST(-0.00637922932685),
    FRAC_CONST(-0.00726158168517), FRAC_CONST(-0.00817982333726),
    FRAC_CONST(-0.00913253296085), FRAC_CONST(-0.01011502154986),
    FRAC_CONST(-0.01113155480321), FRAC_CONST(-0.01218499959508),
    FRAC_CONST(0.01327182200351), FRAC_CONST(0.01439046660792),
    FRAC_CONST(0.01554055533423), FRAC_CONST(0.01673247129989),
    FRAC_CONST(0.01794333813443), FRAC_CONST(0.01918724313698),
    FRAC_CONST(0.02045317933555), FRAC_CONST(0.02174675502535),
    FRAC_CONST(0.02306801692862), FRAC_CONST(0.02441609920285),
    FRAC_CONST(0.02578758475467), FRAC_CONST(0.02718594296329),
    FRAC_CONST(0.02860721736385), FRAC_CONST(0.03005026574279),
    FRAC_CONST(0.03150176087389), FRAC_CONST(0.03297540810337),
    FRAC_CONST(0.03446209487686), FRAC_CONST(0.03596975605542),
    FRAC_CONST(0.03748128504252), FRAC_CONST(0.03900536794745),
    FRAC_CONST(0.04053491705584), FRAC_CONST(0.04206490946367),
    FRAC_CONST(0.04360975421304), FRAC_CONST(0.04514884056413),
    FRAC_CONST(0.04668430272642), FRAC_CONST(0.04821657200672),
    FRAC_CONST(0.04973857556014), FRAC_CONST(0.05125561555216),
    FRAC_CONST(0.05276307465207), FRAC_CONST(0.05424527683589),
    FRAC_CONST(0.05571736482138), FRAC_CONST(0.05716164501299),
    FRAC_CONST(0.0585915683626), FRAC_CONST(0.05998374801761),
    FRAC_CONST(0.06134551717207), FRAC_CONST(0.06268578081172),
    FRAC_CONST(0.06397158980681), FRAC_CONST(0.0652247106438),
    FRAC_CONST(0.06643675122104), FRAC_CONST(0.06760759851228),
    FRAC_CONST(0.06870438283512), FRAC_CONST(0.06976302447127),
    FRAC_CONST(0.07076287107266), FRAC_CONST(0.07170026731102),
    FRAC_CONST(0.07256825833083), FRAC_CONST(0.07336202550803),
    FRAC_CONST(0.07410036424342), FRAC_CONST(0.07474525581194),
    FRAC_CONST(0.07531373362019), FRAC_CONST(0.07580083586584),
    FRAC_CONST(0.07619924793396), FRAC_CONST(0.07649921704119),
    FRAC_CONST(0.07670934904245), FRAC_CONST(0.07681739756964),
    FRAC_CONST(0.07682300113923), FRAC_CONST(0.07672049241746),
    FRAC_CONST(0.07650507183194), FRAC_CONST(0.07617483218536),
    FRAC_CONST(0.07573057565061), FRAC_CONST(0.0751576255287),
    FRAC_CONST(0.07446643947564), FRAC_CONST(0.0736406005762),
    FRAC_CONST(0.07267746427299), FRAC_CONST(0.07158263647903),
    FRAC_CONST(0.07035330735093), FRAC_CONST(0.06896640131951),
    FRAC_CONST(0.06745250215166), FRAC_CONST(0.06576906686508),
    FRAC_CONST(0.06394448059633), FRAC_CONST(0.06196027790387),
    FRAC_CONST(0.0598166570809), FRAC_CONST(0.05751526919867),
    FRAC_CONST(0.05504600343009), FRAC_CONST(0.05240938217366),
    FRAC_CONST(0.04959786763445), FRAC_CONST(0.04663033051701),
    FRAC_CONST(0.04347687821958), FRAC_CONST(0.04014582784127),
    FRAC_CONST(0.03664181168133), FRAC_CONST(0.03295839306691),
    FRAC_CONST(0.02908240060125), FRAC_CONST(0.02503075618909),
    FRAC_CONST(0.02079970728622), FRAC_CONST(0.01637012582228),
    FRAC_CONST(0.01176238327857), FRAC_CONST(0.00696368621617),
    FRAC_CONST(0.00197656014503), FRAC_CONST(-0.00320868968304),
    FRAC_CONST(-0.00857117491366), FRAC_CONST(-0.01412888273558),
    FRAC_CONST(-0.01988341292573), FRAC_CONST(-0.02582272888064),
    FRAC_CONST(-0.03195312745332), FRAC_CONST(-0.03827765720822),
    FRAC_CONST(-0.04478068215856), FRAC_CONST(-0.05148041767934),
    FRAC_CONST(-0.05837053268336), FRAC_CONST(-0.06544098531359),
    FRAC_CONST(-0.07269433008129), FRAC_CONST(-0.08013729344279),
    FRAC_CONST(-0.08775475365593), FRAC_CONST(-0.09555333528914),
    FRAC_CONST(-0.10353295311463), FRAC_CONST(-0.1116826931773),
    FRAC_CONST(-0.120007798468), FRAC_CONST(-0.12850028503878),
    FRAC_CONST(-0.13715517611934), FRAC_CONST(-0.1459766491187),
    FRAC_CONST(-0.15496070710605), FRAC_CONST(-0.16409588556669),
    FRAC_CONST(-0.17338081721706), FRAC_CONST(-0.18281725485142),
    FRAC_CONST(-0.19239667457267), FRAC_CONST(-0.20212501768103),
    FRAC_CONST(-0.21197358538056), FRAC_CONST(-0.22196526964149),
    FRAC_CONST(-0.23206908706791), FRAC_CONST(-0.24230168845974),
    FRAC_CONST(-0.25264803095722), FRAC_CONST(-0.26310532994603),
    FRAC_CONST(-0.27366340405625), FRAC_CONST(-0.28432141891085),
    FRAC_CONST(-0.29507167170646), FRAC_CONST(-0.30590985751916),
    FRAC_CONST(-0.31682789136456), FRAC_CONST(-0.32781137272105),
    FRAC_CONST(-0.33887226938665), FRAC_CONST(-0.3499914122931),
    FRAC_CONST(0.36115899031355), FRAC_CONST(0.37237955463061),
    FRAC_CONST(0.38363500139043), FRAC_CONST(0.39492117615675),
    FRAC_CONST(0.40623176767625), FRAC_CONST(0.41756968968409),
    FRAC_CONST(0.42891199207373), FRAC_CONST(0.44025537543665),
    FRAC_CONST(0.45159965356824), FRAC_CONST(0.46293080852757),
    FRAC_CONST(0.47424532146115), FRAC_CONST(0.48552530911099),
    FRAC_CONST(0.49677082545707), FRAC_CONST(0.50798175000434),
    FRAC_CONST(0.51912349702391), FRAC_CONST(0.53022408956855),
    FRAC_CONST(0.54125534487322), FRAC_CONST(0.55220512585061),
    FRAC_CONST(0.5630789140137), FRAC_CONST(0.57385241316923),
    FRAC_CONST(0.58454032354679), FRAC_CONST(0.59511230862496),
    FRAC_CONST(0.6055783538918), FRAC_CONST(0.61591099320291),
    FRAC_CONST(0.62612426956055), FRAC_CONST(0.63619801077286),
    FRAC_CONST(0.64612696959461), FRAC_CONST(0.65590163024671),
    FRAC_CONST(0.66551398801627), FRAC_CONST(0.67496631901712),
    FRAC_CONST(0.68423532934598), FRAC_CONST(0.69332823767032),
    FRAC_CONST(0.70223887193539), FRAC_CONST(0.71094104263095),
    FRAC_CONST(0.71944626349561), FRAC_CONST(0.72774489002994),
    FRAC_CONST(0.73582117582769), FRAC_CONST(0.74368278636488),
    FRAC_CONST(0.75131374561237), FRAC_CONST(0.75870807608242),
    FRAC_CONST(0.76586748650939), FRAC_CONST(0.77277808813327),
    FRAC_CONST(0.77942875190216), FRAC_CONST(0.7858353120392),
    FRAC_CONST(0.79197358416424), FRAC_CONST(0.797846641377),
    FRAC_CONST(0.80344857518505), FRAC_CONST(0.80876950044491),
    FRAC_CONST(0.81381912706217), FRAC_CONST(0.81857760046468),
    FRAC_CONST(0.82304198905409), FRAC_CONST(0.8272275347336),
    FRAC_CONST(0.8311038457152), FRAC_CONST(0.83469373618402),
    FRAC_CONST(0.83797173378865), FRAC_CONST(0.84095413924722),
    FRAC_CONST(0.84362382812005), FRAC_CONST(0.84598184698206),
    FRAC_CONST(0.84803157770763), FRAC_CONST(0.84978051984268),
    FRAC_CONST(0.85119715249343), FRAC_CONST(0.85230470352147),
    FRAC_CONST(0.85310209497017), FRAC_CONST(0.85357205739107),
    FRAC_CONST(0.85373856005937 /*max*/), FRAC_CONST(0.85357205739107),
    FRAC_CONST(0.85310209497017), FRAC_CONST(0.85230470352147),
    FRAC_CONST(0.85119715249343), FRAC_CONST(0.84978051984268),
    FRAC_CONST(0.84803157770763), FRAC_CONST(0.84598184698206),
    FRAC_CONST(0.84362382812005), FRAC_CONST(0.84095413924722),
    FRAC_CONST(0.83797173378865), FRAC_CONST(0.83469373618402),
    FRAC_CONST(0.8311038457152), FRAC_CONST(0.8272275347336),
    FRAC_CONST(0.82304198905409), FRAC_CONST(0.81857760046468),
    FRAC_CONST(0.81381912706217), FRAC_CONST(0.80876950044491),
    FRAC_CONST(0.80344857518505), FRAC_CONST(0.797846641377),
    FRAC_CONST(0.79197358416424), FRAC_CONST(0.7858353120392),
    FRAC_CONST(0.77942875190216), FRAC_CONST(0.77277808813327),
    FRAC_CONST(0.76586748650939), FRAC_CONST(0.75870807608242),
    FRAC_CONST(0.75131374561237), FRAC_CONST(0.74368278636488),
    FRAC_CONST(0.73582117582769), FRAC_CONST(0.72774489002994),
    FRAC_CONST(0.71944626349561), FRAC_CONST(0.71094104263095),
    FRAC_CONST(0.70223887193539), FRAC_CONST(0.69332823767032),
    FRAC_CONST(0.68423532934598), FRAC_CONST(0.67496631901712),
    FRAC_CONST(0.66551398801627), FRAC_CONST(0.65590163024671),
    FRAC_CONST(0.64612696959461), FRAC_CONST(0.63619801077286),
    FRAC_CONST(0.62612426956055), FRAC_CONST(0.61591099320291),
    FRAC_CONST(0.6055783538918), FRAC_CONST(0.59511230862496),
    FRAC_CONST(0.58454032354679), FRAC_CONST(0.57385241316923),
    FRAC_CONST(0.5630789140137), FRAC_CONST(0.55220512585061),
    FRAC_CONST(0.54125534487322), FRAC_CONST(0.53022408956855),
    FRAC_CONST(0.51912349702391), FRAC_CONST(0.50798175000434),
    FRAC_CONST(0.49677082545707), FRAC_CONST(0.48552530911099),
    FRAC_CONST(0.47424532146115), FRAC_CONST(0.46293080852757),
    FRAC_CONST(0.45159965356824), FRAC_CONST(0.44025537543665),
    FRAC_CONST(0.42891199207373), FRAC_CONST(0.41756968968409),
    FRAC_CONST(0.40623176767625), FRAC_CONST(0.39492117615675),
    FRAC_CONST(0.38363500139043), FRAC_CONST(0.37237955463061),
    FRAC_CONST(-0.36115899031355), FRAC_CONST(-0.3499914122931),
    FRAC_CONST(-0.33887226938665), FRAC_CONST(-0.32781137272105),
    FRAC_CONST(-0.31682789136456), FRAC_CONST(-0.30590985751916),
    FRAC_CONST(-0.29507167170646), FRAC_CONST(-0.28432141891085),
    FRAC_CONST(-0.27366340405625), FRAC_CONST(-0.26310532994603),
    FRAC_CONST(-0.25264803095722), FRAC_CONST(-0.24230168845974),
    FRAC_CONST(-0.23206908706791), FRAC_CONST(-0.22196526964149),
    FRAC_CONST(-0.21197358538056), FRAC_CONST(-0.20212501768103),
    FRAC_CONST(-0.19239667457267), FRAC_CONST(-0.18281725485142),
    FRAC_CONST(-0.17338081721706), FRAC_CONST(-0.16409588556669),
    FRAC_CONST(-0.15496070710605), FRAC_CONST(-0.1459766491187),
    FRAC_CONST(-0.13715517611934), FRAC_CONST(-0.12850028503878),
    FRAC_CONST(-0.120007798468), FRAC_CONST(-0.1116826931773),
    FRAC_CONST(-0.10353295311463), FRAC_CONST(-0.09555333528914),
    FRAC_CONST(-0.08775475365593), FRAC_CONST(-0.08013729344279),
    FRAC_CONST(-0.07269433008129), FRAC_CONST(-0.06544098531359),
    FRAC_CONST(-0.05837053268336), FRAC_CONST(-0.05148041767934),
    FRAC_CONST(-0.04478068215856), FRAC_CONST(-0.03827765720822),
    FRAC_CONST(-0.03195312745332), FRAC_CONST(-0.02582272888064),
    FRAC_CONST(-0.01988341292573), FRAC_CONST(-0.01412888273558),
    FRAC_CONST(-0.00857117491366), FRAC_CONST(-0.00320868968304),
    FRAC_CONST(0.00197656014503), FRAC_CONST(0.00696368621617),
    FRAC_CONST(0.01176238327857), FRAC_CONST(0.01637012582228),
    FRAC_CONST(0.02079970728622), FRAC_CONST(0.02503075618909),
    FRAC_CONST(0.02908240060125), FRAC_CONST(0.03295839306691),
    FRAC_CONST(0.03664181168133), FRAC_CONST(0.04014582784127),
    FRAC_CONST(0.04347687821958), FRAC_CONST(0.04663033051701),
    FRAC_CONST(0.04959786763445), FRAC_CONST(0.05240938217366),
    FRAC_CONST(0.05504600343009), FRAC_CONST(0.05751526919867),
    FRAC_CONST(0.0598166570809), FRAC_CONST(0.06196027790387),
    FRAC_CONST(0.06394448059633), FRAC_CONST(0.06576906686508),
    FRAC_CONST(0.06745250215166), FRAC_CONST(0.06896640131951),
    FRAC_CONST(0.07035330735093), FRAC_CONST(0.07158263647903),
    FRAC_CONST(0.07267746427299), FRAC_CONST(0.0736406005762),
    FRAC_CONST(0.07446643947564), FRAC_CONST(0.0751576255287),
    FRAC_CONST(0.07573057565061), FRAC_CONST(0.07617483218536),
    FRAC_CONST(0.07650507183194), FRAC_CONST(0.07672049241746),
    FRAC_CONST(0.07682300113923), FRAC_CONST(0.07681739756964),
    FRAC_CONST(0.07670934904245), FRAC_CONST(0.07649921704119),
    FRAC_CONST(0.07619924793396), FRAC_CONST(0.07580083586584),
    FRAC_CONST(0.07531373362019), FRAC_CONST(0.07474525581194),
    FRAC_CONST(0.07410036424342), FRAC_CONST(0.07336202550803),
    FRAC_CONST(0.07256825833083), FRAC_CONST(0.07170026731102),
    FRAC_CONST(0.07076287107266), FRAC_CONST(0.06976302447127),
    FRAC_CONST(0.06870438283512), FRAC_CONST(0.06760759851228),
    FRAC_CONST(0.06643675122104), FRAC_CONST(0.0652247106438),
    FRAC_CONST(0.06397158980681), FRAC_CONST(0.06268578081172),
    FRAC_CONST(0.06134551717207), FRAC_CONST(0.05998374801761),
    FRAC_CONST(0.0585915683626), FRAC_CONST(0.05716164501299),
    FRAC_CONST(0.05571736482138), FRAC_CONST(0.05424527683589),
    FRAC_CONST(0.05276307465207), FRAC_CONST(0.05125561555216),
    FRAC_CONST(0.04973857556014), FRAC_CONST(0.04821657200672),
    FRAC_CONST(0.04668430272642), FRAC_CONST(0.04514884056413),
    FRAC_CONST(0.04360975421304), FRAC_CONST(0.04206490946367),
    FRAC_CONST(0.04053491705584), FRAC_CONST(0.03900536794745),
    FRAC_CONST(0.03748128504252), FRAC_CONST(0.03596975605542),
    FRAC_CONST(0.03446209487686), FRAC_CONST(0.03297540810337),
    FRAC_CONST(0.03150176087389), FRAC_CONST(0.03005026574279),
    FRAC_CONST(0.02860721736385), FRAC_CONST(0.02718594296329),
    FRAC_CONST(0.02578758475467), FRAC_CONST(0.02441609920285),
    FRAC_CONST(0.02306801692862), FRAC_CONST(0.02174675502535),
    FRAC_CONST(0.02045317933555), FRAC_CONST(0.01918724313698),
    FRAC_CONST(0.01794333813443), FRAC_CONST(0.01673247129989),
    FRAC_CONST(0.01554055533423), FRAC_CONST(0.01439046660792),
    FRAC_CONST(-0.01327182200351), FRAC_CONST(-0.01218499959508),
    FRAC_CONST(-0.01113155480321), FRAC_CONST(-0.01011502154986),
    FRAC_CONST(-0.00913253296085), FRAC_CONST(-0.00817982333726),
    FRAC_CONST(-0.00726158168517), FRAC_CONST(-0.00637922932685),
    FRAC_CONST(-0.00553372111088), FRAC_CONST(-0.004722259624),
    FRAC_CONST(-0.00394011240522), FRAC_CONST(-0.003193377839),
    FRAC_CONST(-0.00248267236449), FRAC_CONST(-0.00180394725893),
    FRAC_CONST(-0.00115681355227), FRAC_CONST(-0.00054642808664),
    FRAC_CONST(2.760451905E-005), FRAC_CONST(0.0005832264248),
    FRAC_CONST(0.00109023290512), FRAC_CONST(0.00157846825768),
    FRAC_CONST(0.0020274176185), FRAC_CONST(0.00245085400321),
    FRAC_CONST(0.00284467578623), FRAC_CONST(0.00320918858098),
    FRAC_CONST(0.00354012465507), FRAC_CONST(0.00384564081246),
    FRAC_CONST(0.0041251642327), FRAC_CONST(0.00438018617447),
    FRAC_CONST(0.00460395301471), FRAC_CONST(0.004810946906),
    FRAC_CONST(0.00498396877629), FRAC_CONST(0.00513822754514),
    FRAC_CONST(0.00527157587272), FRAC_CONST(0.0053838975897),
    FRAC_CONST(0.0054753783077), FRAC_CONST(0.005540436394),
    FRAC_CONST(0.0055917128663), FRAC_CONST(0.00562661141932),
    FRAC_CONST(0.00563891995151), FRAC_CONST(0.00564551969164),
    FRAC_CONST(0.00562206432097), FRAC_CONST(0.00559380230045),
    FRAC_CONST(0.00554757145088), FRAC_CONST(0.00548760401507),
    FRAC_CONST(0.00541967759307), FRAC_CONST(0.00534716811982),
    FRAC_CONST(0.00524611661324), FRAC_CONST(0.00514073539032),
    FRAC_CONST(0.00503930226013), FRAC_CONST(0.00491376035745),
    FRAC_CONST(0.00479325608498), FRAC_CONST(0.00466064606118),
    FRAC_CONST(0.00452098527825), FRAC_CONST(0.00437307196781),
    FRAC_CONST(0.0042264269227), FRAC_CONST(0.00408197531935),
    FRAC_CONST(0.00392074323703), FRAC_CONST(0.00376039229104),
    FRAC_CONST(0.00360082681231), FRAC_CONST(0.00344188741828),
    FRAC_CONST(0.00327396134847), FRAC_CONST(0.00311254206525),
    FRAC_CONST(0.00294694477165), FRAC_CONST(0.00278704643465),
    FRAC_CONST(0.00262017586902), FRAC_CONST(0.00246256169126),
    FRAC_CONST(0.00230172547746), FRAC_CONST(0.00214615835557),
    FRAC_CONST(0.00198411407369), FRAC_CONST(0.00183482654224),
    FRAC_CONST(0.00168680832531), FRAC_CONST(0.00154432198471),
    FRAC_CONST(0.00139024948272), FRAC_CONST(0.00125778846475),
    FRAC_CONST(0.00112501551307), FRAC_CONST(0.00098859883015),
    FRAC_CONST(0.00086084433262), FRAC_CONST(0.00074580258865),
    FRAC_CONST(0.00062393761391), FRAC_CONST(0.00051073884952),
    FRAC_CONST(0.0004026540216), FRAC_CONST(0.00029495311041),
    FRAC_CONST(0.00020430170688), FRAC_CONST(0.00010943831274),
    FRAC_CONST(1.349497418E-005), FRAC_CONST(-6.173344072E-005),
    FRAC_CONST(-0.00014463809349), FRAC_CONST(-0.0002098337344),
    FRAC_CONST(-0.00028969811748), FRAC_CONST(-0.00035011758756),
    FRAC_CONST(-0.00040951214522), FRAC_CONST(-0.00046063254803),
    FRAC_CONST(-0.00051455722108), FRAC_CONST(-0.00055645763906),
    FRAC_CONST(-0.0005946118933), FRAC_CONST(-0.00063415949025),
    FRAC_CONST(-0.00066504150893), FRAC_CONST(-0.00069179375372),
    FRAC_CONST(-0.00072153919876), FRAC_CONST(-0.00073193571525),
    FRAC_CONST(-0.00075300014201), FRAC_CONST(-0.00076307935757),
    FRAC_CONST(-0.0007757977331), FRAC_CONST(-0.00078014496257),
    FRAC_CONST(-0.000780366471), FRAC_CONST(-0.00077798694927),
    FRAC_CONST(-0.00078343322877), FRAC_CONST(-0.00077248485949),
    FRAC_CONST(-0.0007681371927), FRAC_CONST(-0.00074905980532),
    FRAC_CONST(-0.00074409418541), FRAC_CONST(-0.00072550431222),
    FRAC_CONST(-0.00071577364744), FRAC_CONST(-0.00069416146273),
    FRAC_CONST(-0.00067776907764), FRAC_CONST(-0.00065403333621),
    FRAC_CONST(-0.00063124935319), FRAC_CONST(-0.00061327473938),
    FRAC_CONST(-0.00058709304852), FRAC_CONST(-0.00056778025613),
    FRAC_CONST(-0.00054665656337), FRAC_CONST(-0.00052265642972),
    FRAC_CONST(-0.00050407143497), FRAC_CONST(-0.00048937912498),
    FRAC_CONST(-0.00048752279712), FRAC_CONST(-0.00049475180896),
    FRAC_CONST(-0.00056176925738), FRAC_CONST(-0.00055252865047)
};

#endif

