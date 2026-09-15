import requests
import sys
import hashlib
from requests_toolbelt import MultipartEncoder

def upload_firmware(ip, bin_path):
    upload_url = f"http://{ip}"
    try:
        print(f"Inizio upload del firmware {bin_path} verso {upload_url}...")
        with open(bin_path, 'rb') as firmware:
            md5 = hashlib.md5(firmware.read()).hexdigest()
            firmware.seek(0)
            
            start_url = f"{upload_url}/ota/start?mode=fr&hash={md5}"
            print(f"Chiamata a {start_url}...")
            start_res = requests.get(start_url)
            print(f"Start Response: {start_res.status_code}")
            
            if start_res.status_code != 200:
                print("Failed to start OTA")
                return
                
            encoder = MultipartEncoder(fields={
                'MD5': md5,
                'firmware': ('firmware', firmware, 'application/octet-stream')}
            )
            
            post_headers = {
                'Content-Type': encoder.content_type,
            }
            
            print("Caricamento del file in corso...")
            response = requests.post(f"{upload_url}/ota/upload", data=encoder, headers=post_headers)
            
            print(f"Status Code: {response.status_code}")
            print(f"Response: {response.text}")
            
            if response.status_code == 200:
                print("Upload completato con successo! Il dispositivo si riavvierà a breve.")
            else:
                print("Errore durante l'upload.")
    except Exception as e:
        print(f"Eccezione: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python ota_upload.py <ip> <bin_path>")
        sys.exit(1)
    upload_firmware(sys.argv[1], sys.argv[2])
