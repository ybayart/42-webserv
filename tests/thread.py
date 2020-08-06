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

		url = "http://localhost:8080/f"
		urls = {
			"get": url
		}
		headers = {
			"connection" : "keep-alive",
		    "keep-alive" : "timeout=10, max=10"
		};
		max = 100
		for x in range(max):
			try:
				response = requests.get(urls[self.method], headers = headers)
				if response.text != "404: File not found\n":
					print("==STATUSCODE==")
					print(response.status_code)
					print("==HEADERS==")
					print(response.headers)
			except:
				print("connection reset by peer")

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

