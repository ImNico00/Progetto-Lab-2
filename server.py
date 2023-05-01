#! /usr/bin/env python3
# server che fornisce l'elenco dei primi in un dato intervallo 
# gestisce più clienti contemporaneamente usando i thread
# invia il byte inutile all'inizio della connessione
import struct, socket, threading, concurrent.futures, argparse, os.path, subprocess, time, signal, logging


# host e porta di default
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 57447  # Port to listen on (non-privileged ports are > 1023)
PIPESCRITTORI = "caposc"
PIPELETTORI = "capolet"
Max_sequence_length = 2048
Description ="""Server sgravato"""

def main(nThread, nLettori, nScrittori, valgrind, host, port):
  #Creo le pipe nel caso non esistano
  if not os.path.exists(PIPESCRITTORI):
    os.mkfifo(PIPESCRITTORI,0o666)
  
  if not os.path.exists(PIPELETTORI):
    os.mkfifo(PIPELETTORI,0o666)
    
  #Se v è uguale a None allora vuol dire che non è stato scelto
  if valgrind == False:
    p = subprocess.Popen(["./archivio", nLettori, nScrittori])
  else:
    p = subprocess.Popen(["valgrind","--leak-check=full", 
                          "--show-leak-kinds=all", 
                          "--log-file=valgrind-%p.log", 
                          "./archivio", nLettori, nScrittori])

  print("Ho lanciato il processo:", p.pid)
  print("Attendo 5 secondi (nel frattempo arriva l'output di archivio)")
  try:
    time.sleep(5)
  except KeyboardInterrupt:
    print("Attesa accorciata da SIGINT")


  pipeW = os.open(PIPESCRITTORI, os.O_WRONLY)
  pipeR = os.open(PIPELETTORI, os.O_WRONLY)
  fileLog = open("server.log", "w")
  logging.basicConfig(stream=fileLog, level=logging.INFO)

  # creiamo il server socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:  
      s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      s.bind((host, port))
      s.listen()
      lock = threading.Lock()
      with concurrent.futures.ThreadPoolExecutor(max_workers=nThread) as executor:
        while True:
          #print("SERVER - In attesa di un client...")
          # mi metto in attesa di una connessione
          conn, addr = s.accept()
          # l'esecuzione di submit non è bloccante
          # fino a quando ci sono thread liberi
          executor.submit(gestisci_connessione, conn, addr, pipeW, pipeR, lock)
          #gestisci_connessione(conn, addr, pipeW, pipeR)
    except KeyboardInterrupt:
      pass
    print('Va bene smetto...')
    print("Chiudo pipe...")
    os.close(pipeW)
    os.close(pipeR)
    s.shutdown(socket.SHUT_RDWR)
    # mando un segnale SIGTERM a main
    p.send_signal(signal.SIGTERM)
    time.sleep(10) # diamo il tempo al segnale di arrivare
    # ora p.poll() restituisce l'exit code di main
    print("Exit code di main o None se non è terminato:", p.poll())
  print("Cancello le pipes...")
  os.unlink(PIPESCRITTORI)
  os.unlink(PIPELETTORI)
  fileLog.close()
  print("[SERVER] PROGRAMMA TERMINATO!")
    

# gestisci una singola connessione con un client
def gestisci_connessione(conn, addr, pipeW, pipeR, lock): 
  # in questo caso potrei usare direttamente conn
  # e l'uso di with serve solo a garantire che
  # conn venga chiusa all'uscita del blocco
  # ma in generale with esegue le necessarie
  # inzializzazione e il clean-up finale
  # TIPO A -> LETTORI
  # TIPO B -> SCRITTORI
  with conn:
    logging.info(f"{threading.current_thread().name} contattato da {addr}")
    #print(f"{threading.current_thread().name} contattato da {addr}")
    # ---- attendo due interi, 1 per il tipo e l'altro per lunghezza sequenza
    data = recv_all(conn,8)
    assert len(data) == 8
    tipo = struct.unpack("!i",data[:4])[0] # 1 se tipo A - 2 se tipo B
    if tipo == 1:
      lenSequenza = struct.unpack("!i",data[4:])[0]
      data = recv_all(conn,lenSequenza)
      #QUA DEVE ARRIVARE LA STRINGA DI LUNGHEZZA STABILITA
      assert len(data)==lenSequenza
      #sequenza = data.decode()
      #print(f"Ho ricevuto la sequenza:", sequenza)
      #print("Scrivo su pipe...")
      lock.acquire()
      os.write(pipeR, struct.pack("<i",len(data)))
      print("SERVER TIPO 1 - Ho scritto",len(data),"bytes")
      os.write(pipeR, data)
      lock.release()
      logging.info(f"Scritti {len(data)} bytes nella pipe capolet")
      #print("Finito di scrivere su pipe...")
    else:
      data = recv_all(conn, Max_sequence_length)
      #QUI NON SI SA PERCIO' NON FACCIO UN ASSERT PER CONTROLLARE MA CONTROLLO SOLO CHE NON SIA TROPPO GROSSA
      assert len(data) > 0
      assert len(data) <= Max_sequence_length
      #sequenza = data.decode()
      #print(f"Ho ricevuto la sequenza:", sequenza)
      #print("Scrivo su pipe...")
      lock.acquire()
      os.write(pipeW, struct.pack("<i",len(data)))
      print("SERVER TIPO 2 - Ho scritto", len(data),"bytes")
      os.write(pipeW, data)
      lock.release()
      logging.info(f"Scritti {len(data)} bytes nella pipe caposc")
      #print("Finito di scrivere su pipe...")
    
  #print(f"{threading.current_thread().name} finito con {addr}")
 


# riceve esattamente n byte e li restituisce in un array di byte
# il tipo restituto è "bytes": una sequenza immutabile di valori 0-255
# analoga alla readn che abbiamo visto nel C
def recv_all(conn,n):
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n:
    chunk = conn.recv(min(n - bytes_recd, Max_sequence_length))
    if len(chunk) == 0:
      #print("Arrivata la linea di lunghezza 0...")
      break
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks


# questo codice viene eseguito solo se il file è eseguito direttamente
# e non importato come modulo con import da un altro file
if __name__ == '__main__':
  # parsing della linea di comando vedere la guida
  #    https://docs.python.org/3/howto/argparse.html
  parser = argparse.ArgumentParser(description=Description, formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument('threads', help="numero di thread da utilizzare", type = int)  
  parser.add_argument("-r", "-lettori", 
                      help="numero di lettori da utilizzare", type = int, 
                      default=3)
  parser.add_argument("-w", "-scrittori", 
                      help="numero di scrittori da utilizzare", type = int, 
                      default=3)
  parser.add_argument("-a", help='host address', type = str, 
                      default=HOST)  
  parser.add_argument("-p", help='port', type = int, 
                      default=PORT) 
  parser.add_argument("-v", "-valgrind", 
                      help='per attivare Valgrind', action='store_true') 
  args = parser.parse_args()
  assert args.threads > 0, "Il numero di thread deve essere maggiore di 0"
  assert args.r > 0 and args.w > 0, "Il numero per ogni thread deve essere > 0"
  main(args.threads, f"{args.r}", f"{args.w}", args.v, args.a, args.p)
