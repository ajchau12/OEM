server {
	server_name kicad.olinelectricmotorsports.com kicad.olinelectricmotorsports.com;
	
	location / {
		include uwsgi_params;
		uwsgi_pass unix:/olin-electric-motorsports/infrastructure/kicad_website/kicad.sock;
	}

    listen 443 ssl; # managed by Certbot
    ssl_certificate /etc/letsencrypt/live/kicad.olinelectricmotorsports.com/fullchain.pem; # managed by Certbot
    ssl_certificate_key /etc/letsencrypt/live/kicad.olinelectricmotorsports.com/privkey.pem; # managed by Certbot
    include /etc/letsencrypt/options-ssl-nginx.conf; # managed by Certbot
    ssl_dhparam /etc/letsencrypt/ssl-dhparams.pem; # managed by Certbot


}
server {
    if ($host = kicad.olinelectricmotorsports.com) {
        return 301 https://$host$request_uri;
    } # managed by Certbot


	listen 80;
	server_name kicad.olinelectricmotorsports.com kicad.olinelectricmotorsports.com;
    return 404; # managed by Certbot


}