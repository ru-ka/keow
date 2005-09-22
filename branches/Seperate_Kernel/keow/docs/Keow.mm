<map version="0.8.0 RC5">
<!-- To view this file, download free mind mapping software FreeMind from http://freemind.sourceforge.net -->
<node CREATED="1125379481933" ID="Freemind_Link_1359664046" MODIFIED="1127332077665" TEXT="Keow">
<icon BUILTIN="licq"/>
<icon BUILTIN="penguin"/>
<node CREATED="1127332273877" ID="Freemind_Link_1020862138" MODIFIED="1127332277632" POSITION="left" TEXT="Memory Management">
<node CREATED="1127332305622" ID="Freemind_Link_1639156445" MODIFIED="1127332325791" TEXT="Kernel allocates the memory">
<node CREATED="1127332327594" ID="Freemind_Link_1991957296" MODIFIED="1127332338079" TEXT="Injects memory with VirtualAllocEx()"/>
<node CREATED="1127332361062" ID="Freemind_Link_1287660079" MODIFIED="1127332414419" TEXT="mmap sometimes is not a map&#xa;(because of different page sizes)"/>
<node CREATED="1127332564174" ID="Freemind_Link_1104348431" MODIFIED="1127332579246" TEXT="Uses code injection to do MapViewOfFile in the process"/>
</node>
</node>
<node CREATED="1127332122679" ID="Freemind_Link_1844939002" MODIFIED="1127332127426" POSITION="left" TEXT="IO handling">
<node CREATED="1127332586897" ID="Freemind_Link_1841349600" MODIFIED="1127332601278" TEXT="Reads/Writes done in the User process"/>
<node CREATED="1127332603581" ID="Freemind_Link_999647488" MODIFIED="1127332620946" TEXT="Console IO re-directed thru the kernel">
<node CREATED="1127332622979" ID="Freemind_Link_1400949900" MODIFIED="1127332630480" TEXT="Console is a seperate process"/>
<node CREATED="1127332630950" ID="Freemind_Link_1616874688" MODIFIED="1127332683215" TEXT="Kernel needs to see the data&#xa;so it can handle Ctrl-C generating a signal&#xa;(Before the process is ready to read data)"/>
</node>
</node>
<node CREATED="1127332846210" ID="Freemind_Link_548382610" MODIFIED="1127332863935" POSITION="right" TEXT="Doc">
<icon BUILTIN="attach"/>
</node>
<node CREATED="1127332817198" ID="_" MODIFIED="1127332826181" POSITION="right" TEXT="TODO">
<icon BUILTIN="flag"/>
</node>
<node CREATED="1127332108409" ID="Freemind_Link_801234826" MODIFIED="1127332118373" POSITION="left" TEXT="Process Seperation">
<node CREATED="1127332129629" ID="Freemind_Link_671840355" MODIFIED="1127332134787" TEXT="Kernel Process">
<node CREATED="1127332181875" ID="Freemind_Link_627446056" MODIFIED="1127332188604" TEXT="Runs the &apos;kernel&apos;"/>
<node CREATED="1127332189556" ID="Freemind_Link_1430063751" MODIFIED="1127332202554" TEXT="Manages processes by being a debugger to them"/>
<node CREATED="1127332204016" ID="Freemind_Link_675832327" MODIFIED="1127332215903" TEXT="Intercepts INT 80h system calls"/>
<node CREATED="1127332218367" ID="Freemind_Link_204538099" MODIFIED="1127332436100" TEXT="Uses code injection to get process to&#xa;do process specific work (eg mmap)"/>
</node>
<node CREATED="1127332135989" ID="Freemind_Link_455489581" MODIFIED="1127332148777" TEXT="User processes">
<node CREATED="1127332168005" ID="Freemind_Link_361424672" MODIFIED="1127332452534" TEXT="Runs real x86 Linux ELF code"/>
<node CREATED="1127332250934" ID="Freemind_Link_1444252626" MODIFIED="1127332260037" TEXT="One per linux process"/>
<node CREATED="1127332457010" ID="Freemind_Link_1706185045" MODIFIED="1127332468296" TEXT="Must be i386 code (so we know about INT80)"/>
</node>
<node CREATED="1127332149328" ID="Freemind_Link_1187857434" MODIFIED="1127332154665" TEXT="IO Processes">
<node CREATED="1127332157960" ID="Freemind_Link_561712955" MODIFIED="1127332164870" TEXT="Console Handler">
<node CREATED="1127332474015" ID="Freemind_Link_880182957" MODIFIED="1127332500653" TEXT="NT can only have one console per process&#xa;so to get many windows we need many processes">
<node CREATED="1127332508564" ID="Freemind_Link_751904310" MODIFIED="1127332536585" TEXT="Being seperate also allows later running of keow apps inside normal console windows"/>
</node>
</node>
</node>
</node>
</node>
</map>
