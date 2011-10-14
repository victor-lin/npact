# Create your views here.
import logging
import os.path
import tempfile
import urllib2

from pynpact import util, entrez, prepare

from django import forms
from django.conf import settings
from django.contrib import messages
from django.core.urlresolvers import reverse
from django.http import HttpResponseRedirect
from django.shortcuts import render_to_response
from django.template import RequestContext


from spatweb import is_clean_path, getabspath, getrelpath, library_root
from spatweb.middleware import RedirectException

#used to dynamically load functions.
import start

# Get an instance of a logger
logger = logging.getLogger(__name__)


def mksavefile(prefix):
    """Wrapper around creating the file to save uploaded files in.

  Returns the (fd,path) tuple (similar to tempfile.mkstemp)
"""
    #we're using tempfile to ensure the file we get is unique and
    #aren't overwriting another.
    fd,abspath = tempfile.mkstemp(dir=settings.MEDIA_ROOT, prefix=prefix, suffix=".gbk")
    relpath = getrelpath(abspath)
    return (fd,abspath,relpath)



            # elif form.cleaned_data.get('url') :
            #     path = pullFromUrl(req, form.cleaned_data.get('url'))
            # elif form.cleaned_data.get('pastein') :
            #     path = saveToFile(req, form.cleaned_data.get('pastein'))

class UploadForm(forms.Form):
    type="UploadForm"
    title="Upload a File"
    file_upload = forms.FileField(required=True)

    def clean(self):
        cleaned_data = self.cleaned_data
        fu = cleaned_data.get('file_upload')
        if fu:
            logger.info("Checking Uploaded file %s", fu)
            if not is_clean_path(fu.name):
                raise forms.ValidationError("Illegal filename")

            fd,savepath,relpath = mksavefile( "up-%s-" % fu.name)
            logger.info("Saving uploaded file to %r", relpath)
            with os.fdopen(fd, 'wb') as fh :
                for chunk in fu.chunks() :
                    fh.write(chunk)

            cleaned_data['path']=relpath
        return cleaned_data

class UrlForm(forms.Form):
    type="UrlForm"
    title="From URL"
    url = forms.URLField(label="From URL", required=True)

    def clean(self):
        cleaned_data = self.cleaned_data
        url = cleaned_data.get('url')
        if url:
            logger.debug("Going to pull from %r", url)
            pull_req = urllib2.Request(url)
            if pull_req.get_type == "file":
                raise forms.ValidationError("Illegal URL")
            try:
                fh = urllib2.urlopen(pull_req)
                fd,savepath,relpath = mksavefile("url")
                util.stream_to_file(fh,savepath)
                cleaned_data['path'] = relpath
            except:
                logger.exception("Error fetching url %s", url)
                raise forms.ValidationError("Error fetching url")

        return cleaned_data

class PasteForm(forms.Form):
    type="PasteForm"
    title="Paste as Text"
    pastein = forms.CharField(label="Paste in as text",
                              widget=forms.Textarea(attrs={'rows':3, 'cols':""}),
                              required=True)
    def clean(self):
        cleaned_data = self.cleaned_data
        text = cleaned_data.get('pastein')
        if text:
            (fd,savepath,relpath) = mksavefile("txt")
            logger.info("Saving paste to %r", relpath)
            cleaned_data['path'] = relpath
            with os.fdopen(fd,'wb') as fh :
                fh.write(text)
            return cleaned_data

class EntrezSearchForm(forms.Form):
    type="EntrezSearchForm"
    title="Search Entrez"
    template="start/EntrezSearchForm.html"
    term = forms.CharField(label="Search Term",required=True)

    def clean(self):
        logger.debug("Starting handling Entrez search.")
        cleaned_data = self.cleaned_data
        if cleaned_data.get('term'): 
            self.session = entrez.CachedEntrezSession(library_root())

            self.session.search(cleaned_data['term'])
            logger.debug("Search finished, found %d matches", self.session.result_count)
            if self.session.result_count == 1:
                cleaned_data['path'] = getrelpath(self.session.fetch())
            elif self.session.result_count > 1:
                raise forms.ValidationError("Too many results (%d) found, need 1. Try refining the search or searching for a RefSeq id." % (self.session.result_count))
            else:
                raise forms.ValidationError("No results found.")
        return cleaned_data


def build_forms(req):
    if req.method == 'POST':
        types = [UploadForm,UrlForm,PasteForm,EntrezSearchFormPOST]
        active = req.POST['active']
        cls = getattr(start, active, None)
        return [ cls() ]

        #forms = []
        # for i,type in zip(range(len(types)),types):
        #     if i == active:
        #         forms.append(type(req.POST,req.FILES))
        #     else:
        #         forms.append(type())

        # return forms
    else:
        types = [UploadForm,UrlForm,PasteForm,EntrezSearchForm]
        return [type() for type in types]


def view(req) :
    form = None
    if req.method == 'POST' :
        path=None
        active = req.POST['active']
        cls = getattr(start, active)
        submitted = cls(req.POST, req.FILES)
        if submitted.is_valid():
            logger.info("Form is valid")
            path = submitted.cleaned_data['path']
            raise RedirectException(reverse('run',args=[path]))
        else :
            forms = [submitted]
    else:
        types = [UploadForm,UrlForm,PasteForm,EntrezSearchForm]
        forms = [type() for type in types]
        active = ''

    return render_to_response('start.html',{'forms':forms, 'active':active},
                                  context_instance=RequestContext(req))


def re_search(req):
    if req.REQUEST.get('term'):
        esf = EntrezSearchForm(req.REQUEST)
    return render_to_response('start.html',{'forms':[esf], 'active':'EntrezSearchForm'},
                                  context_instance=RequestContext(req))


def efetch(req, id):
    logger.info("Asked to fetch Id: %s", id)
    session = entrez.EntrezSession(library_root())
    abspath = session.fetch_id(id)

    try:
        prepare.try_parse(abspath)
    except prepare.InvalidGBKException, e:
        messages.error(req, str(e))
        return re_search(req)
    except:
        messages.error(request,"There was a problem loading file '%s', please try again or try a different record." % path)
        return re_search(req)
    
    return HttpResponseRedirect(reverse('run',args=[getrelpath(abspath)]))
