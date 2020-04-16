# Subscribe
Subscription using a server and TCP and UDP clients

## UDP clients
Codul pentru acesti clienti a facut parte din schelet si a fost folosit numai pentru testare,
asa ca acesta nu se va regasi in continutul temei.
Acestia vor publia mesaje pe diferite topicuri.

## TCP clients
Acestia se vor conecta la server printr-o metoda "subscribe topic" si se vor putea dezabona printr-o
metoda similara, "unsubscribe"

## Server
Cand acesta primeste mesaje de la clientii de UDP pe diverse topicuri, acesta le va trimite clientilor
abonati la acel topic. In plus, serverul se va ocupa de abonarea si dezabonarea clientilor.
