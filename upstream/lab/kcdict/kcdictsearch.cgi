#! /usr/bin/ruby
# -*- coding: utf-8 -*-

require 'cgi'

CMDNAME = 'kcdictmgr'
DICTS =
  [
   [ 'dict.kct', '' ],
#   [ 'wordnet.kct', 'WordNet' ],
#   [ 'eijiro-eiji.kct', 'Eijiro' ],
#   [ 'eijiro-waei.kct', 'Waeijiro' ],
#   [ 'eijiro-ryaku.kct', 'Ryakugoro' ],
#   [ 'eijiro-reiji.kct', 'Reijiro' ],
  ]
DICTGUESS = false
TEXTSIMPLEMATCH = true
ENV['PATH'] = (ENV['PATH'] || '') + ':/usr/local/bin:.:..:../..'
ENV['LD_LIBRARY_PATH'] = (ENV['LD_LIBRARY_PATH'] || '') + ':/usr/local/lib:.:..:../..'

param = CGI.new
scriptname = ENV['SCRIPT_NAME']
scriptname = $0.sub(/.*\//, '') if !scriptname
p_query = (param['q'] || '').strip
p_dict = param['d'] || ''
p_dict = p_dict.empty? ? -1 : p_dict.to_i
p_mode = (param['m'] || '').strip
p_mode = 'p' if p_mode.empty?
p_num = (param['n'] || '').to_i
p_num = 30 if p_num < 1
p_num = 10000 if p_num > 10000
p_skip = (param['s'] || '').to_i
p_skip = 0 if p_skip < 0
p_skip = 65536 if p_skip > 65536
p_type =(param['t'] || '').strip

if DICTGUESS && p_dict < 0
  didx = p_query.match(/^[\x00-\x7f]+$/) ? 0 : 1
  dict = DICTS[didx]
else
  p_dict = 0 if p_dict < 0
  p_dict = 0 if p_dict >= DICTS.length
  dict = DICTS[p_dict]
end

ctype = p_type == 'x' ? 'application/xml' : 'text/html; charset=UTF-8'
printf("Content-Type: %s\r\n", ctype)
printf("\r\n")

print <<__EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<meta http-equiv="Content-Style-Type" content="text/css" />
<meta http-equiv="Content-Script-Type" content="text/javascript" />
<title>Dictionary Search</title>
<style tyle="text/css">html, body {
  margin: 0em 0em;
  background: #ffffff;
}
div#inputpanel {
  margin: 0em 0em;
  padding: 0.8em 1em;
  background: #eef8ff;
  border-bottom: solid 1px #ddeeff;
}
div#mainpanel {
  margin: 0em 0em;
  padding: 0.8em 1em;
}
a {
  color: #1144ee;
}
h1.logo {
  margin: 0.2em 0em;
  font-size: 110%;
}
h1.logo a {
  color: #aaddee;
  text-decoration: none;
}
div.credit {
  margin-bottom: 0.5em;
  padding: 0.5em 1em;
  text-align: right;
  font-size: 80%;
  color: #777777;
}
div.credit a {
  color: #777777;
  text-decoration: none;
}
dl#result dt {
  line-height: 110%;
}
dl#result dt {
  margin: 0.5em 0em 0.1em 0em;
}
dl#result dt span.word {
  color: #001177;
}
dl#result dt span.seq {
  font-size: 70%;
  color: #777788;
}
dl#result dt span.part, dl#result dt span.dist {
  font-size: 80%;
  color: #666677;
}
dl#result dd {
  margin: 0.2em 0em 0.4em 3em;
}
dl#result dd.text {
  font-size: 95%;
  color: #333333;
}
div#navi {
  margin: 0.5em 0.5em;
  font-size: 95%;
}
div#navi a {
  margin: 0em 0.5em;
}
</style>
</head>
<body>
__EOF

printf("<div id=\"inputpanel\">\n")
printf("<h1 class=\"logo\"><a href=\"%s\">Dictionary Search</a></h1>\n",
       CGI.escapeHTML(scriptname))
printf("<form action=\"%s\" method=\"GET\">\n", CGI.escapeHTML(scriptname))
printf("<input type=\"text\" name=\"q\" value=\"%s\" size=\"32\" />\n", CGI.escapeHTML(p_query))
if DICTS.length > 1
  printf("<select name=\"d\">\n")
  if DICTGUESS
    printf("<option value=\"-1\"%s>auto</option>\n",
           p_dict < 0 ? ' selected="selected"' : '')
  end
  dseq = 0
  DICTS.each do |pair|
    printf("<option value=\"%d\"%s>%s</option>\n",
           dseq, dseq == p_dict ? ' selected="selected"' : '', CGI.escapeHTML(pair[1]))
    dseq += 1
  end
  printf("</select>\n")
end
printf("<select name=\"m\">\n")
[ [ 'p', 'prefix' ], [ 'f', 'full' ], [ 'a', 'ambiguous' ], [ 'm', 'middle' ],
  [ 'r', 'regex' ], [ 'tm', 'text middle' ], [ 'tr', 'text regex' ] ].each do |pair|
  mode = pair[0]
  printf("<option value=\"%s\"%s>%s</option>\n",
         mode, mode == p_mode ? ' selected="selected"' : '', pair[1])
end
printf("</select>\n")
printf("<select name=\"n\">\n")
[ 1, 5, 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000 ].each do |num|
  printf("<option value=\"%d\"%s>%d</option>\n",
         num, num == p_num ? ' selected="selected"' : '', num)
end
printf("</select>\n")
printf("<input type=\"submit\" value=\"search\" />\n")
printf("</form>\n")
printf("</div>\n")

printf("<div id=\"mainpanel\">\n")
if !p_query.empty?
  case p_mode
  when 'f'
    mode = '-f'
  when 'a'
    mode = '-a'
  when 'm'
    mode = '-m'
  when 'r'
    mode = '-r'
  when 'tm'
    mode = '-tm'
  when 'tr'
    mode = '-tr'
  else
    mode = ''
  end
  tsmode = TEXTSIMPLEMATCH ? '-ts' : ''
  cmd = sprintf('%s search -max %d %s %s -iu -- %s "%s"',
                CMDNAME, p_num + p_skip + 1, mode, tsmode, dict[0], CGI.escape(p_query))
  records = []
  IO.popen(cmd) do |io|
    io.readlines.each do |line|
      line.force_encoding('utf-8')
      line = line.strip
      fields = line.split("\t")
      if fields.length >= 3
        dist = p_mode == 'a' && fields.length >= 4 ? fields[3] : nil
        record = [ fields[0], fields[1], fields[2], dist ]
        records.push(record)
      end
    end
  end
  if records.empty?
    printf("<p class=\"notice\">no matching</p>\n")
    if p_mode == 'p' || p_mode == 'f'
      printf("<p class=\"suggestion\"><a href=\"%s?q=%s&amp;d=%d&amp;m=a&amp;n=%d&amp;s=%d\">" +
             "ambiguous search for \"%s\"</a></p>\n",
             scriptname, CGI.escape(p_query), p_dict, p_num, p_skip - p_num,
             CGI.escapeHTML(p_query))
    end
  else
    isnext = records.length > p_num + p_skip
    records = records[p_skip, p_num] || []
    seq = p_skip
    printf("<dl id=\"result\">\n")
    records.each do |record|
      seq += 1
      printf("<dt id=\"r%d\">", seq)
      printf("<span class=\"seq\">%d:</span> ", seq)
      printf("<span class=\"word\">%s</span>", CGI.escapeHTML(record[0]))
      if !record[1].empty?
        printf(" <span class=\"part\">[%s]</span> ", CGI.escapeHTML(record[1]))
      end
      if record[3]
        printf(" <span class=\"dist\">(%d)</span> ", record[3])
      end
      printf("</dt>\n")
      printf("<dd class=\"text\">%s</dd>\n", CGI.escapeHTML(record[2]))
    end
    printf("</dl>\n")
    if p_skip > 0 || isnext
      printf("<div id=\"navi\">\n")
      if p_skip > 0
        printf("<a href=\"%s?q=%s&amp;d=%d&amp;m=%s&amp;n=%d&amp;s=%d\">PREV</a>\n",
               scriptname, CGI.escape(p_query), p_dict, CGI.escape(p_mode),
               p_num, p_skip - p_num)
      end
      if isnext
        printf("<a href=\"%s?q=%s&amp;d=%d&amp;m=%s&amp;n=%d&amp;s=%d\">NEXT</a>\n",
               scriptname, CGI.escape(p_query), p_dict, CGI.escape(p_mode),
               p_num, p_skip + p_num)
      end
      printf("</div>\n")
    end
  end
end
printf("</div>\n")

print <<__EOF
<div class="credit">powered by <a href="http://fallabs.com/kyotocabinet/">Kyoto Cabinet</a></div>
</body>
</html>
__EOF
