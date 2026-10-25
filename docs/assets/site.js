document.addEventListener('DOMContentLoaded', () => {
  // Scroll reveal - subtle, 200ms
  const reveal = new IntersectionObserver((entries) => {
    entries.forEach(e => { if (e.isIntersecting) e.target.classList.add('in'); });
  }, { threshold: 0.15, rootMargin: '0px 0px -32px 0px' });
  document.querySelectorAll('.reveal').forEach(el => reveal.observe(el));

  // Active nav link
  const path = location.pathname.split('/').pop() || 'index.html';
  document.querySelectorAll('.nav-links a, #mobile-menu a').forEach(a => {
    const href = a.getAttribute('href');
    if (href && href.split('/').pop() === path) a.classList.add('active');
  });

  // Mobile menu — class-based with ARIA state
  const toggle = document.querySelector('.mobile-toggle');
  const menu = document.getElementById('mobile-menu');
  if (toggle && menu) {
    toggle.addEventListener('click', () => {
      const isOpen = menu.classList.contains('open');
      menu.classList.toggle('open', !isOpen);
      menu.classList.toggle('closed', isOpen);
      toggle.setAttribute('aria-expanded', String(!isOpen));
    });
  }

  // Tabs - consistent interaction pattern
  document.querySelectorAll('.tabs').forEach(tabs => {
    const container = tabs.parentElement;
    const panels = container.querySelectorAll('.tab-panel');
    const buttons = tabs.querySelectorAll('.tab');
    buttons.forEach((btn, i) => {
      btn.addEventListener('click', () => {
        if (btn.classList.contains('tab-disabled') || btn.disabled) return;
        buttons.forEach(b => b.classList.remove('active'));
        panels.forEach(p => p.classList.remove('active'));
        btn.classList.add('active');
        if (panels[i]) panels[i].classList.add('active');
      });
    });
  });

  // Theme toggle — with aria-pressed state
  const themeToggle = document.querySelector('.theme-toggle');
  if (themeToggle) {
    const syncPressed = () => {
      const current = document.documentElement.getAttribute('data-theme');
      const isDark = current === 'dark' ||
        (!current && window.matchMedia('(prefers-color-scheme: dark)').matches);
      themeToggle.setAttribute('aria-pressed', String(isDark));
    };
    syncPressed();
    themeToggle.addEventListener('click', () => {
      const current = document.documentElement.getAttribute('data-theme');
      const isDark = current === 'dark' ||
        (!current && window.matchMedia('(prefers-color-scheme: dark)').matches);
      const next = isDark ? 'light' : 'dark';
      document.documentElement.setAttribute('data-theme', next);
      localStorage.setItem('meshflow-theme', next);
      syncPressed();
    });
  }

  // Mobile docs sidebar toggle
  const docsToggle = document.querySelector('.docs-mobile-toggle');
  const docsSidebar = document.querySelector('.docs-sidebar');
  const docsOverlay = document.querySelector('.docs-overlay');
  if (docsToggle && docsSidebar) {
    docsToggle.addEventListener('click', () => {
      docsSidebar.classList.toggle('open');
      if (docsOverlay) docsOverlay.classList.toggle('open');
    });
    if (docsOverlay) {
      docsOverlay.addEventListener('click', () => {
        docsSidebar.classList.remove('open');
        docsOverlay.classList.remove('open');
      });
    }
  }

  // Reading progress bar
  const progress = document.querySelector('.reading-progress');
  if (progress) {
    const updateProgress = () => {
      const scrolled = window.scrollY;
      const total = document.documentElement.scrollHeight - window.innerHeight;
      progress.style.width = total > 0 ? (scrolled / total * 100) + '%' : '0%';
    };
    window.addEventListener('scroll', updateProgress, { passive: true });
    updateProgress();
  }

  // Scroll to top button
  const scrollTop = document.querySelector('.scroll-top');
  if (scrollTop) {
    window.addEventListener('scroll', () => {
      scrollTop.classList.toggle('visible', window.scrollY > 400);
    }, { passive: true });
    scrollTop.addEventListener('click', () => {
      window.scrollTo({ top: 0, behavior: 'smooth' });
    });
  }

  // Copy buttons on code blocks
  document.querySelectorAll('pre').forEach(pre => {
    if (pre.querySelector('.code-copy')) return;
    const btn = document.createElement('button');
    btn.className = 'code-copy';
    btn.textContent = 'Copy';
    btn.addEventListener('click', () => {
      const code = pre.querySelector('code');
      navigator.clipboard.writeText(code ? code.textContent : pre.textContent).then(() => {
        btn.textContent = 'Copied';
        btn.classList.add('copied');
        setTimeout(() => { btn.textContent = 'Copy'; btn.classList.remove('copied'); }, 2000);
      });
    });
    pre.appendChild(btn);
  });

  // ═══ Number Ticker — count-up animation ═══
  const counters = document.querySelectorAll('[data-count]');
  const counterObserver = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (!entry.isIntersecting) return;
      const el = entry.target;
      const target = parseInt(el.dataset.count, 10);
      const duration = 1500;
      const start = performance.now();
      const animate = (now) => {
        const elapsed = now - start;
        const progress = Math.min(elapsed / duration, 1);
        const eased = 1 - Math.pow(1 - progress, 3);
        el.textContent = Math.round(target * eased);
        if (progress < 1) requestAnimationFrame(animate);
      };
      requestAnimationFrame(animate);
      counterObserver.unobserve(el);
    });
  }, { threshold: 0.5 });
  counters.forEach(c => counterObserver.observe(c));

  // ═══ Magic Card — spotlight mouse tracking ═══
  document.querySelectorAll('.magic-card').forEach(card => {
    card.addEventListener('pointermove', (e) => {
      const rect = card.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;
      card.style.setProperty('--mx', x + 'px');
      card.style.setProperty('--my', y + 'px');
    });
  });

  // ═══ FAQ Accordion ═══
  document.querySelectorAll('.faq-item').forEach(item => {
    const question = item.querySelector('.faq-question');
    const answer = item.querySelector('.faq-answer');
    if (!question || !answer) return;
    question.addEventListener('click', () => {
      const isOpen = item.classList.contains('open');
      // Close all other items
      document.querySelectorAll('.faq-item.open').forEach(other => {
        if (other !== item) {
          other.classList.remove('open');
          other.querySelector('.faq-answer').style.maxHeight = '0';
          other.querySelector('.faq-question').setAttribute('aria-expanded', 'false');
        }
      });
      // Toggle current
      item.classList.toggle('open', !isOpen);
      question.setAttribute('aria-expanded', String(!isOpen));
      answer.style.maxHeight = isOpen ? '0' : answer.scrollHeight + 'px';
    });
  });

  // ═══ Interactive Demo — hotspot switching ═══
  const demoHotspots = document.querySelectorAll('.demo-hotspot');
  const demoScreens = document.querySelectorAll('.demo-screen');
  const demoCursor = document.getElementById('demo-cursor');
  const demoInfo = document.getElementById('demo-info');
  const demoTextServer = document.getElementById('demo-text-server');
  const demoTextClient = document.getElementById('demo-text-client');

  const demoStates = {
    cursor: {
      info: 'Move your cursor to the edge of the server screen and it <strong>seamlessly appears</strong> on the client. No buttons, no shortcuts.',
      server: '$ meshflow --server<br>Listening on :24800<br><span class="typed">Cursor crossed to client.</span>',
      client: '$ meshflow --client<br>Connected to server<br><span class="typed">Receiving input...</span>',
      activeScreen: 'client',
      cursorTarget: { x: 320, y: 60 }
    },
    clipboard: {
      info: 'Copy text on one machine, paste on another. The <strong>clipboard syncs automatically</strong> as the cursor crosses screen boundaries.',
      server: '$ meshflow --server<br>Listening on :24800<br><span class="typed">Clipboard sent to client.</span>',
      client: '$ meshflow --client<br>Connected to server<br><span class="typed">Clipboard received.</span>',
      activeScreen: 'client',
      cursorTarget: { x: 320, y: 60 }
    },
    encrypt: {
      info: 'All traffic between server and client is <strong>encrypted with TLS</strong>. Certificates are generated and managed automatically.',
      server: '$ meshflow --server<br>TLS handshake complete<br><span class="typed">Channel encrypted.</span>',
      client: '$ meshflow --client<br>Verifying fingerprint...<br><span class="typed">TLS established.</span>',
      activeScreen: 'server',
      cursorTarget: { x: 100, y: 60 }
    }
  };

  function switchDemo(state) {
    const data = demoStates[state];
    if (!data) return;

    demoHotspots.forEach(h => h.classList.toggle('active', h.dataset.demo === state));
    demoScreens.forEach(s => s.classList.toggle('active', s.dataset.screen === data.activeScreen));

    if (demoInfo) demoInfo.innerHTML = data.info;
    if (demoTextServer) demoTextServer.innerHTML = data.server;
    if (demoTextClient) demoTextClient.innerHTML = data.client;

    if (demoCursor && data.cursorTarget) {
      demoCursor.style.transform = `translate(${data.cursorTarget.x}px, ${data.cursorTarget.y}px)`;
    }
  }

  demoHotspots.forEach(hotspot => {
    hotspot.addEventListener('click', () => switchDemo(hotspot.dataset.demo));
  });

  // Auto-rotate demo states
  let demoIndex = 0;
  const demoKeys = Object.keys(demoStates);
  let demoInterval = null;

  function startDemoRotation() {
    if (demoInterval) clearInterval(demoInterval);
    demoInterval = setInterval(() => {
      demoIndex = (demoIndex + 1) % demoKeys.length;
      switchDemo(demoKeys[demoIndex]);
    }, 5000);
  }

  function stopDemoRotation() {
    if (demoInterval) { clearInterval(demoInterval); demoInterval = null; }
  }

  if (demoHotspots.length > 0) {
    const demoContainer = document.querySelector('.demo-container');
    if (demoContainer) {
      const demoObs = new IntersectionObserver((entries) => {
        entries.forEach(e => {
          if (e.isIntersecting) startDemoRotation();
          else stopDemoRotation();
        });
      }, { threshold: 0.3 });
      demoObs.observe(demoContainer);

      demoContainer.addEventListener('pointerenter', stopDemoRotation);
      demoContainer.addEventListener('pointerleave', startDemoRotation);
    }
  }

  // ═══ Footer year ═══
  const yearEl = document.getElementById('footer-year');
  if (yearEl) yearEl.textContent = new Date().getFullYear();
});
