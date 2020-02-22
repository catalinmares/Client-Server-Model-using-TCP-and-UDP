Mares Catalin-Constantin
322 CD
Protocoale de comunicatie
Tema 2 - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

ABORDARE GENERALA
Pentru aceasta tema am decis sa folosesc liste... foarte multe liste. 
In primul rand, avem o lista de clienti. Aceasta este formata doar din
clientii TCP care se conecteaza la server pentru a primi mesaje de la
clientii UDP. Un client nou poate fi adaugat in lista in urma acceptarii
unei noi conexiuni pe socket-ul pentru conexiuni TCP. Un client se poate
deconecta in orice moment de la server, caz in care acesta va ramane in
lista de clienti pentru o eventuala reconectare. Cand un client se
deconecteaza, se retine in lista faptul ca din momentul deconectarii
pana la o eventuala reconectare, clientul este deconectat. Fac acest
lucru prin 2 campuri ale structurii ClientList, si anume connected si
socket. In momentul in care un client se connecteaza, populez campurile
client_ID, IP, port, socket si connected. Cand un client se deconecteaza,
modific in lista de clienti ca nodul specific acelui client sa aiba
socket-ul setat la -1 si connected la 0. Cand acel client se reconecteaza,
identific nodul specific clientului inainte de reconectare pe baza ID-ului
si actualizez campurile IP, port, socket si setez din nou connected la 1.
Fiecare client conectat la server poate comunica cu acesta prin comenzile
"subscribe" si "unsubscribe". In momentul in care un client trimite
serverului un request de "subscribe", un nou topic va fi adaugat in lista
sa de topic-uri si va ramane acolo pana cand un nou request de "unsubscribe"
va fi trimis serverului de catre acel client. 
	Astfel, un client are o lista de topic-uri la care este abonat, iar
fiecare topic are o optiune de S&F si o lista de mesaje stocate. In momentul
in care un client se aboneaza la anumite topic-uri cu SF = 1 si apoi se
deconecteaza, toate mesajele pentru topic-urile respective primite de la 
clientii UDP vor fi inserate in lista de mesaje a topic-ului ce apartine
listei de topic-uri la care clientul este abonat si vor ramane acolo pana
cand clientul se va reconecta sau serverul isi va incheia conexiunea.

SUBSCRIBER-UL
Subscriber-ul (clientul TCP) va verifica in prima instanta ca parametrii
primiti la rulare sa fie suficienti si valizi. Va deschide o conexiune
catre server si va trimite imediat serverului ID-ul clientului care
doreste sa se conecteze. Daca ID-ul nu este deja folosit de alt client
conectat, conexiunea va ramane deschisa, altfel serverul va trimite un
mesaj de eroare, caz in care clientul va inchide conexiunea. 
	De asemenea, clientul se mai ocupa de parsarea si validarea
comenzilor primite de la tastatura. Daca acestea sunt valide, clientul
le va transmite serverului.
	In final, clientul va astepta mesaje de la server si le va afisa
la consola. Acestea sunt mesajele provenite de la clientii TCP cu
topic-urile la care clientul este abonat.

SERVERUL
Serverul va verifica in prima instanta ca parametrii primiti la rulare sa
fie suficienti si valizi. Va deschide un 2 socketi, unul pe care va "asculta"
conexiuni TCP si unul pe care va primi mesaje de la clientii UDP. 
	Serverul, ca si clientii TCP, functioneaza pana cand se primeste
de la tastatura comanda "exit", aceasta fiind singura comanda acceptata de
server. Serverul va accepta noi conexiuni pe portul pe care "asculta" si
va adauga noii clienti in lista (sau va actualiza in caz de reconectare) 
si le va trimite clientilor reconectati mesajele stocate in lipsa lor. 
	In cazul primirii de mesaje de la clientii UDP, serverul va parsa
mesajele si le va formata conform cerintei clientilor TCP, dupa care le 
va trimite acestora in cazul in care sunt conectati sau le va stoca in
lista acestora de mesaje pana cand acestia se vor reconecta.
	Atunci cand un client TCP trimite un request de "subscribe", 
serverul adauga noul topic la lista de topic-uri a clientului si orice
mesaj cu acel topic va fi stocat intr-o lista unde va ramane pana cand
acel client se va reconecta si va primi mesajele stocate. 
	Atunci cand un client TCP trimite un request de "unsubscribe", 
serverul elimina topic-ul respectiv din lista de topic-uri a clientului.