import requests
import time
import random
import sys
from threading import Thread

random.seed()
class Client(Thread):
	"""Thread chargé simplement d'afficher une lettre dans la console."""

	def __init__(self, method):
		Thread.__init__(self)
		self.method = method

	def run(self):
		"""Code à exécuter pendant l'exécution du thread."""

		url = "http://localhost:8080/"
		urls = {
			"get": url + "f",
			"post": url + "",
			"put": url + "put_test/test.txt",
			"cgi": url + "directory/youpi.bla"
		}
		body = "loin de moi l'idée de graver dans le marbre de tailler dans une écorce d'arbre loin de moi l'idée de suggérer que je m'en moque que je n'en ai rien à faire que guère je ne m'en soucie loin de moi ces folies mais je m'échine depuis octobre et pourquoi donc depuis début octobre même et qui m'aime me suive depuis octobre depuis ce même dernier octobre le trois du mois je crois depuis ce temps-là depuis trois mois depuis trois mois et une semaine je m'échine ailleurs et le très long texte n'a pas avancé d'un poil pas beaucoup sans doute est-ce mon côté velléitaire qui ne cesse de me jouer des tours et les méandres du très long texte se sont figés comme une gelée le long des parois d'un bocal de verre et je vitupère contre mes essais éphémères mon tempérament affreusement velléitaire et ce teint d'albâtre qui n'est pas le mien comme je voudrais qu'il fût d'albâtre ou d'ébène ou autrement même sans métaphore mais au moins qu'il ait quelque tenue que mon visage sans retenue puisse soudain passer pour un tissu une pierre un songe soit en quelque sorte un tableau fasse tableau mais ce n'est pas le cas même ce mot albâtre jeté au visage jeté tout à trac sur la page en haut de page ce mot me défigure ne me figure pas ne me représente pas ne figure rien de ce que je suis de ce que je pense être et je suis encore et toujours circonspect dans le doute et ce mot n'apporte rien aucune réponse et donc toujours je me jette à la figure ces accusations comme des bouteilles non pas à la mer mais bien dans la gueule oui je me donne des coups de bouteille tessons épars sur le parquet et mes joues ensanglantées enfin que ce soit ou non métaphore que le mot d'albâtre me figure ou non je prends ces coups ces reproches en plein visage et je m'accuse d'être velléitaire aussi bien sûr pour trop entreprendre je lance cent feux il est normal qu'un certain nombre des foyers meure et même ne démarre qu'à peine avant de s'achever dans un bruit de feuilles mouillées de bois mort de bois trop vert encore pour prendre tout cela encore métaphore et toujours métaphore peut-être est-ce le mot albâtre qui appelle autant de métaphores ou bien les conditions d'écriture du très long texte que par facétie ou encore autodérision je pourrais être tenté de rebaptiser très long texte interrompu et l'adjectif interrompu ici au milieu de la ligne interrompt mes songes interrompt le torrent de sornettes lance d'autres tirades propose peut-être d'autres charades mais pour mieux me ramener vers le rivage bourbeux où je ne cesse de me lancer ces reproches à la figubles même si certaines sont justes par ailleurs comme dans le cas du projet de traduire régulièrement et pensais-je au début au moins une fois par semaine un poème et qui s'est enlisé après à peine trois ou quatre tracasseries mais cela reprendra parfois aussi depuis début octobre le trois je crois suspendu à ce mot d'albâtre depuis le trois octobre le trois je crois je me disais que pour être interrompu ou inachevé le très long texte recelait de vraies possibilités et qu'il suffisait suffirait eût suffi de s'y remettre et la machine reprendrait du galon non là cette image-là ne va pas je mélange les formules croise les figures de style et donc je pensais qu'il me faudrait toutes proportions gardées envisager ces carnets comme Paul Valéry travaillant régulièrement et sans espoir d'en finir jamais chaque matin à ses Cahiers désormais regroupés en deux tomes en Pléiade et que j'ai dévorés consultés admirés lus compulsés longuement naguère mais il faudrait dire jadis ou balancer entre les deux lus disons entre 1993 et 1997 et donc toutes proportions gardées je me verrais bien ainsi à reprendre tel chantier interrompu trois mois et le faisant avancer un petit peu mais enfin ce n'est pas possible il ne va pas se comparer à Paul Valéry l'autre oiseux oisif ex-oisien de surcroît ancien oisien into the bargain non il ne va pas se comparer à Paul Valéry tout de même alors que seulement et il nous l'a dit même avec métaphores tout le tintouin oui oui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreuxui noir sur blanc dit ce n'est rien d'autre qu'un affreux"
		headers = {
			"connection" : "keep-alive",
		};
		max = 5
		for x in range(max):
			rdm = random.randint(1, 4)
			if rdm == 1:
				response = requests.get(urls[self.method], headers = headers)
				if response.text != "404: File not found\n":
					print(self.method + "error")
				# else:
				# 	print(self.method + "ok")
			if rdm == 2:
				response = requests.post(urls[self.method], headers = headers)
				if response.text != "405: Method Not Allowed\n":
					print(self.method + "error")

			if rdm == 3:
				# print("before put")
				response = requests.put(urls[self.method], data = body)
				print(response.status_code)
			if rdm == 4:
				response = requests.post(urls["cgi"], data = body)
				if response.status_code != 200:
					print(response.status_code)
		# 	print("done")
		# print("all done")
				# print(response.text)

				# else:
				# 	print(self.method + "ok")

# Création des threads
threads = []

for x in range(300):
	threads.append(Client("get"))

# Lancement des threads
for x in threads:
	x.start()

for x in threads:
	x.join()
# Attend que les threads se terminent

